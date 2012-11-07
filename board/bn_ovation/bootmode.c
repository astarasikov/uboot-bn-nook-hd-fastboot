#include <common.h>
#include <part_efi.h>
#include <mmc.h>
#include <asm/arch/sys_info.h>
#include <twl6030.h>
#include <bn_boot.h>

#define BN_BOOTLIMIT     8
#define RESET_REASON ( ( * ( (volatile unsigned int *) ( 0x4A307B04 ) ) ) & 0x3 )
#define WARM_RESET ( 1 << 1 )
#define COLD_RESET ( 1 )

#define HOME_BUTTON	32
#define POWER_BUTTON	29

#ifdef BOARD_CHARGING
extern int boot_cooldown_charger_mode(void);
#endif

struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[1024];
};

static const struct bootloader_message romrestore_bcb = {
	.command = "boot-recovery",
	.status = "",
	.recovery = "recovery\n--update_package=/factory/romrestore.zip\n--restore=rom\n",
};

static const struct bootloader_message factory_bcb = {
	.command = "boot-recovery",
	.status = "",
	.recovery = "recovery\n--update_package=/factory/factory.zip\n--restore=factory\n",
};

static const struct bootloader_message master_clear_bcb = {
	.command = "boot-recovery",
	.status = "",
	.recovery = "recovery\n--wipe_data_ui\n",
};

static inline int load_serial_num(void)
{
	memset((void*)0x81000000, 0, 32);
	if (!run_command("mmcinit 1; fatload mmc 1:4 0x81000000 devconf/DeviceId 31", 0)) {
		setenv("serialnum",(char *)0x81000000);
		setenv("dieid#",(char *)0x81000000);
		return 0;
	}

	return -1;
}

static inline int read_bcb(void)
{
	return run_command("mmcinit 1; fatload mmc 1:5 0x81000000 BCB 2048", 0);
}

static void write_bcb(const struct bootloader_message * const bcb)
{
	char buf[64];
	sprintf(buf, "mmcinit 1; fatsave mmc 1:5 0x%08x BCB", bcb);
	run_command(buf, 0);
}

/* return 1 if file exists on fatfs: */
static int check_fat_file_exists(int slot , int partition, char * file) {
	volatile unsigned int* ptr = (unsigned int*) 0x80000000;
	char buffer[64];
	int ret = 0;

	*ptr = 0xDEADBEEF;
	sprintf(buffer, "mmcinit %d; fatload mmc %d:%d 0x%08p %s 4", slot, slot, partition, ptr, file);

	if (run_command(buffer, 0) == 0 && *ptr != 0xDEADBEEF) {
		ret = 1;
	}
	return ret;

}

extern int32_t FB;
static void set_boot_cmd( int boot_type)
{
	char buffer[256];
	sprintf(buffer, "setenv bootargs ${bootargs} boot.fb=%x", FB);
	if ( SD_BOOTIMG == boot_type ) {
		run_command("setenv bootargs ${sdbootargs}", 0);
		setenv ("bootcmd", "mmcinit 0; fatload mmc 0:1 0x81000000 flashing_boot.img; booti 0x81000000");
		setenv ("altbootcmd", "run bootcmd"); // for sd boot altbootcmd is the same as bootcmd
	} else if ( SD_UIMAGE == boot_type ){
		run_command("setenv bootargs ${sdbootargs}", 0);
		setenv ("bootcmd", "mmcinit 0; fatload mmc 0 0x80000000 uImage; bootm 0x80000000");
	} else if ( EMMC_ANDROID == boot_type ) {
		run_command(buffer, 0);
		setenv ("bootcmd", "mmcinit 1; booti mmc1");
	} else if ( EMMC_RECOVERY == boot_type ) {
		setenv("bootcmd", "mmcinit 1; booti mmc1 recovery");
	}
#ifdef CONFIG_USBBOOT
	else if (USB_BOOTIMG == boot_type) { // device boot from USB, use a simple args
		setenv ("bootargs","androidboot.console=ttyO0 console=ttyO0,115200n8 init=/init rootwait vram=32M omapfb.vram=0:32M");
		setenv ("bootcmd", "booti 0x81000000");
		setenv ("altbootcmd", "run bootcmd");
	}
#endif
}

static void set_recovery_update_bcb(void)
{
	static struct bootloader_message bcb = {
		.command = "boot-recovery",
		.status = "",
		.recovery = "",
	};

	sprintf(bcb.recovery, "recovery\n--update_package=/sdcard/%s\n--update_factory\n", "ovation_update.zip");
	write_bcb(&bcb);
	return;
}

int check_emmc_boot_mode(void)
{
	static int ret_val = EMMC_ANDROID;
	static int has_checked_bootmode_from_fs = 0;
	struct bootloader_message * bcb_ptr;

	//we need to run this again to avoid data abort if reading ppz from mmc 1:5
	if ( check_fat_file_exists( 0 , 1, "ovation_update.zip") ){
		if ( RESET_REASON != WARM_RESET ){
			set_recovery_update_bcb();
			ret_val = EMMC_RECOVERY;
		}
	}

#ifndef TI_EXTERNAL_BUILD
	if(!has_checked_bootmode_from_fs) {
		has_checked_bootmode_from_fs = 1;

		if ( EMMC_ANDROID == ret_val ) {
			if (!read_bcb()) {
				bcb_ptr = (struct bootloader_message *) 0x81000000;
				printf("BCB found, checking...\n");

				if (bcb_ptr->command[0] != 0 &&
						bcb_ptr->command[0] != 255) {
					printf("Booting into recovery\n");
					ret_val = EMMC_RECOVERY;
				}
			} else {
				printf("No BCB found, recovery mode forced.\n");
				ret_val = EMMC_RECOVERY;
			}

			if (load_serial_num()) {
				printf("No serialnum found, rom restore forced.\n");
				write_bcb(&romrestore_bcb);
				ret_val = EMMC_RECOVERY;
			}

		}
	}
#endif

	if ( EMMC_ANDROID == ret_val ) {
		u8 pwron = 0;
		if (twl6030_hw_status(&pwron)) {
			printf("Failed to read twl6030 hw_status\n");
		}

		// Check master clear button press combination (power+home)
		// note that home button is inverted
		if ((gpio_read(HOME_BUTTON) == 0) &&
				(pwron & STS_PWRON) != STS_PWRON) {
			printf("Master Clear forced, booting into recovery\n");
			write_bcb(&master_clear_bcb);
			ret_val = EMMC_RECOVERY;
		}
	}

#ifdef BOARD_CHARGING
	if (boot_cooldown_charger_mode())
		ret_val = EMMC_RECOVERY;
#endif

	return ret_val;
}



int set_boot_mode(void)
{
	int ret = 0;
	int dev = CFG_FASTBOOT_MMC_NO;
	char boot_dev_name[8];

	unsigned int boot_device = get_boot_device(boot_dev_name);

	printf("Booting from: %s\n", boot_dev_name);
#ifdef CONFIG_USBBOOT
	if (boot_device == BOOT_DEVICE_USB) {
		ret = USB_BOOTIMG;
		set_boot_cmd( USB_BOOTIMG );
	}
	else
#endif
	if (boot_device == BOOT_DEVICE_SD) {
		if (!check_fat_file_exists(0, 1, "uImage")) {
			ret = SD_BOOTIMG;
			set_boot_cmd( SD_BOOTIMG );
		} else {
			/*
			 * This is only to to allow external builds to boot,
			 * once security is enabled, this will be pulled out
			 */
			ret = SD_UIMAGE;
			set_boot_cmd( SD_UIMAGE ) ;
		}
	} else {
#if 0
		/* It should be "mmcinit 1". But run_command("mmcinit 0/1") always returns 1.
		 * It means "mmcinit" is reapeatable.
		 * So efi_load_ptble() is always not executed here.
		 * u-boot prints efi partiotion table infomation when execute "mmcinit 1" in read_bcb();
		 * /

		/* Init mmc 0 before loading efi table */
		if (run_command("mmcinit 0", 0) >= 0 ) {
			ret = efi_load_ptbl(dev);
		}
#endif
		if ( 0 == ret ) {
			if ( EMMC_ANDROID == check_emmc_boot_mode() ) {
				ret = EMMC_ANDROID;
				set_boot_cmd( EMMC_ANDROID );
			} else {
				ret = EMMC_RECOVERY;
				set_boot_cmd( EMMC_RECOVERY );
			}
		}
	}

	return ret;
}

#ifdef CONFIG_BOOTCOUNT_LIMIT
unsigned long bootcount_load(void)
{
	/*
	 * Set bootcount to limit+1 per default,
	 * in case we fail to read it apply factory fallback
	 */
	unsigned long bootcount = BN_BOOTLIMIT + 1;
	char boot_dev_name[8];
	char buf[64];
#ifdef CONFIG_USBBOOT
	if (get_boot_device(boot_dev_name) == BOOT_DEVICE_USB) {
		printf("USB boot do not load BootCnt!\n");
		bootcount = 0;
		return bootcount;
	}
#endif

	setenv("bootlimit", stringify(BN_BOOTLIMIT));
	setenv("altbootcmd", "booti mmc1 recovery");
	if ( get_boot_device(boot_dev_name) != BOOT_DEVICE_SD ) {

		sprintf(buf, "mmcinit 1; fatload mmc 1:5 0x%08x BootCnt 4", &bootcount);
		if (run_command(buf, 0)) {
			printf("No BootCnt found, rom restore forced.\n");
			write_bcb(&romrestore_bcb);
		}
	} else {
		bootcount = 0;
	}
	return bootcount;
}

void bootcount_store(unsigned long bootcount)
{
	char boot_dev_name[8];
	char buf[64];
#ifdef CONFIG_USBBOOT
	if ( get_boot_device(boot_dev_name) == BOOT_DEVICE_USB ) {
		printf("USB boot do not save BootCnt!\n");
		return;
	}
#endif

	if ( get_boot_device(boot_dev_name) != BOOT_DEVICE_SD ) {

		printf("BootCnt %lu\n", bootcount);
		if (bootcount > BN_BOOTLIMIT) {
			/*
			 * In case we have reached the bootlimit
			 * we write the factory restore bcb
			 */
			write_bcb(&factory_bcb);

			/* and to prevent us from applying the factory
			 * fallback for infinity we clear it before entering recovery
			 */
			bootcount = 0;
		}
		sprintf(buf, "fatsave mmc 1:5 0x%08x BootCnt", &bootcount);
		if (run_command(buf, 0)) {
			printf("Cannot write BootCnt, rom restore forced.\n");
			write_bcb(&romrestore_bcb);
		}
	}
}
#endif
