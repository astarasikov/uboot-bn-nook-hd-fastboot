/*
 * Copyright (c) 2012 Barnes & Noble Inc.
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 */

#include <common.h>
#include <part_efi.h>
#include <mmc.h>
#include <fastboot.h>
#include <twl6030.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/gpio.h>

#define RESET_REASON ( ( * ( (volatile unsigned int *) ( 0x4A307B04 ) ) ) & 0x3 )
#define WARM_RESET ( 1 << 1 )
#define COLD_RESET ( 1 )

#define HOME_BUTTON		32
#define POWER_BUTTON	29

#ifdef TI_EXTERNAL_BUILD
struct partition {
	const char *name;
	unsigned start_kb; /* partition offset. 0 means continue from previous */
	unsigned size_kb;  /* partition size.   0 means autosize to the end of media */
};

static const struct partition partitions[] = {
	/* name       start_kb    size_kb */
	{ "xloader",       128,       128 }, /* xloader must start at 128 KB */
	{ "bootloader",      0,       256 },
	{ "recovery",        0,     15872 },
	{ "boot",      16*1024,   16*1024 },
	{ "rom",             0,   48*1024 },
	{ "bootdata",        0,   48*1024 },
	{ "factory",         0,  370*1024 },
	{ "system",          0,  612*1024 },
	{ "cache",           0,  426*1024 },
	{ "media",           0, 1024*1024 },
	{ "userdata",        0,         0 }, /* autosize to the end */
	{},
};
#endif

int fastboot_oem(const char *cmd)
{
#ifdef TI_EXTERNAL_BUILD
	if (!strcmp(cmd, "format"))
		return efi_do_format(partitions, CFG_FASTBOOT_MMC_NO);
#endif
	return -1;
}

void board_mmc_init(void)
{
	/* nothing to do this early */
}

struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[1024];
};
// Shared sprintf buffer for fatsave
static char buf[64];

static inline int read_bcb(void)
{
	return run_command("mmcinit 1; fatload mmc 1:5 0x81000000 BCB 2048", 0);
}

static void write_bcb(const struct bootloader_message * const bcb)
{
	sprintf(buf, "mmcinit 1; fatsave mmc 1:5 0x%08x BCB", bcb);
	run_command(buf, 0);
}

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

#ifdef CONFIG_BOOTCOUNT_LIMIT
unsigned long bootcount_load(void)
{
	if (running_from_sd()) {
		return 0;
	}

	unsigned long bootcount = ACCLAIM_BOOTLIMIT + 1; // Set bootcount to limit+1 per default, in case we fail to read it apply factory fallback 

	sprintf(buf, "mmcinit 1; fatload mmc 1:5 0x%08x BootCnt 4", &bootcount);
	if (run_command(buf, 0)) {
		printf("No BootCnt found, rom restore forced.\n");
		write_bcb(&romrestore_bcb);
	}
	return bootcount;
}

void bootcount_store(unsigned long bootcount)
{
	if (running_from_sd()) {
		return;
	}

	printf("BootCnt %lu\n", bootcount);
	if (bootcount > ACCLAIM_BOOTLIMIT) {
		// In case we have reached the bootlimit
		// we write the factory restore bcb
		write_bcb(&factory_bcb);

		// and to prevent us from applying the factory
		// fallback for infinity we clear it before entering recovery
		bootcount = 0;
	}

	sprintf(buf, "mmcinit 1; fatsave mmc 1:5 0x%08x BootCnt", &bootcount);
	if (run_command(buf, 0)) {
		printf("Cannot write BootCnt, rom restore forced.\n");
		write_bcb(&romrestore_bcb);
	}
}
#endif

void do_factory_fallback(void)
{
	write_bcb(&factory_bcb);
	run_command("mmcinit 1; booti mmc1 recovery", 0);
}

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

static const char * const update_zip_names[] = {
	"acclaim_update.zip",
	"evt2acclaim_update.zip",
	"evt2siacclaim_update.zip"
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static int check_update_zip(void)
{
	volatile unsigned int *update_image_location = (unsigned int *) 0x80000000;
	char buffer[64];
	int i;

	*update_image_location  = 0xDEADBEEF;

	for (i = 0; i < ARRAY_SIZE(update_zip_names); ++i) {
		sprintf(buffer, "mmcinit 0; fatload mmc 0 0x80000000 %s 4", update_zip_names[i]);
	
		if (!run_command (buffer, 0) &&
			*update_image_location != 0xDEADBEEF) {
			return i;
		}
	}

	return -1;
}

static int check_for_uimage(void)
{
	volatile unsigned int* uimage_test_location = (unsigned int*) 0x80000000;
	char buffer[64];
	int ret_val = -1;

	*uimage_test_location = 0xDEADBEEF;
	sprintf(buffer, "mmcinit 0; fatload mmc 0 0x80000000 uImage 4");

	if ( (!run_command ( buffer ,0) ) &&
		( *uimage_test_location != 0xDEADBEEF ) ){
		ret_val = 1;
	}
	return ret_val;
}

enum boot_action {
	BOOT_SD,
	RECOVERY,
	BOOT_EMMC,
	INVALID,
};

static inline enum boot_action get_boot_action(void) 
{
	static struct bootloader_message update_bcb = {
		.command = "boot-recovery",
		.status = "",
		.recovery = "",
	};

	static const struct bootloader_message master_clear_bcb = {
		.command = "boot-recovery",
		.status = "",
		.recovery = "recovery\n--wipe_data_ui\n",
	};

	volatile unsigned int *reset_reason = (unsigned int *) 0x4A307B04;
	volatile struct bootloader_message *bcb = (struct bootloader_message *) 0x81000000;
	static const char reboot_panic[] = "reboot\0panic";

	u8 pwron = 0;
	int update_zip;

	if (!memcmp((const char *) PUBLIC_SAR_RAM_1_FREE, reboot_panic, sizeof(reboot_panic))) {
		printf("REBOOT DUE TO KERNEL PANIC!\n");
	}

	// First check for sd boot
	if (running_from_sd()) {
		printf("Booting from sd\n");
		return BOOT_SD;
	}

	if (mmc_init(1)) {
		printf("mmc_init failed!\n");
		return INVALID;
	}
#ifdef TI_EXTERNAL_BUILD
	return BOOT_EMMC;
#else
	if (load_serial_num()) {
		printf("No serialnum found, rom restore forced.\n");
		write_bcb(&romrestore_bcb);
		return RECOVERY;
	}

	fastboot_flash_dump_ptn();

	// Then check if there's a BCB file

	if (!read_bcb()) {
		printf("BCB found, checking...\n");
			
		if (bcb->command[0] != 0 &&
			bcb->command[0] != 255) {
			printf("Booting into recovery\n");
			return RECOVERY;
		}
	} else {
		printf("No BCB found, recovery mode forced.\n");
		return RECOVERY;
	}

	// If cold reboot/start
	if (!(*reset_reason & WARM_RESET) && 
		strcmp((const char *) PUBLIC_SAR_RAM_1_FREE, "reboot")) {

		// Then check for update zip on sd
		update_zip = check_update_zip();

		if (update_zip >= 0 && update_zip < ARRAY_SIZE(update_zip_names)) {
			sprintf(update_bcb.recovery, "recovery\n--update_package=/sdcard/%s\n--update_factory\n", update_zip_names[update_zip]);
			write_bcb(&update_bcb);
			printf("Found %s, booting into recovery\n", update_zip_names[update_zip]);
			return RECOVERY;
		}
	} else if (!strcmp((const char *) PUBLIC_SAR_RAM_1_FREE, "recovery")) {
		printf("Rebooted with recovery reason, booting into recovery\n");
		return RECOVERY;
	}

	if (twl6030_hw_status(&pwron)) {
		printf("Failed to read twl6030 hw_status\n");
	}

	// Check master clear button press combination (power+home)
	// note that home button is inverted
	if ((gpio_read(HOME_BUTTON) == 0) &&
		(pwron & STS_PWRON) != STS_PWRON) {
		printf("Master Clear forced, booting into recovery\n");
		write_bcb(&master_clear_bcb);
		return RECOVERY;
	}

	printf("Booting into Android\n");
	return BOOT_EMMC;
#endif
}

int determine_boot_type(void)
{
	setenv("bootlimit", stringify(ACCLAIM_BOOTLIMIT));
	setenv("altbootcmd", "mmcinit 1; booti mmc1 recovery");

	switch(get_boot_action()) {
	case BOOT_SD:
		if (check_for_uimage() != 1) {
			run_command("setenv bootargs ${sdbootargs}", 0);
			setenv("bootcmd", "mmcinit 0; fatload mmc 0:1 0x81000000 flashing_boot.img; booti 0x81000000");
			setenv("altbootcmd", "run bootcmd"); // for sd boot altbootcmd is the same as bootcmd
		} else {
			run_command("setenv bootargs ${sdbootargs}", 0);
			setenv("bootcmd", "mmcinit 0; fatload mmc 0 0x80000000 uImage; bootm 0x80000000");
		}
		break;
	case RECOVERY:
		setenv("bootcmd", "mmcinit 1; booti mmc1 recovery");
		break;
	case BOOT_EMMC:
		setenv("bootcmd", "mmcinit 1; booti mmc1 boot");
		break;
	case INVALID:
	default:
		printf("Aborting boot!\n");
		return 1;
	}

	return 0;
}

