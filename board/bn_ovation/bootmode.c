#include <common.h>
#include <part_efi.h>
#include <mmc.h>
#include <asm/arch/sys_info.h>
#include <twl6030.h>
#include <bn_boot.h>
#include <omap4430sdp_lcd.h>

static void set_boot_cmd( int boot_type)
{
	memset((void*)0x81000000, 0, 32);
	if (!run_command("mmcinit 1; fatload mmc 1:4 0x81000000 devconf/DeviceId 31", 0)) {
		setenv("serialnum",(char *)0x81000000);
		setenv("dieid#",(char *)0x81000000);
	}
	
	memset((void*)0x81000000, 0, 51);
	if (!run_command("mmcinit 1; fatload mmc 1:4 0x81000000 devconf/DisplayVendor 50", 0)) {
		setenv("display_vendor",(char *)0x81000000);
	}
	
	char buffer[256];
	sprintf(buffer, "setenv bootargs ${sdbootargs} androidboot.hardware=hummingbird display.vendor=AUO boot.fb=%x", ONSCREEN_BUFFER);
	run_command(buffer, 0);
	setenv ("bootcmd", "mmcinit 0; fatload mmc 0:1 0x81000000 kernel ; fatload mmc 0:1 82000000 ramdisk; bootm 0x81000000 0x82000000");
	setenv ("altbootcmd", "run bootcmd");

	run_command("fastboot", 0);
}

int check_emmc_boot_mode(void)
{
	return SD_UIMAGE;
}

int set_boot_mode(void)
{
	char boot_dev_name[8];

	int boot_device = SD_UIMAGE;
	set_boot_cmd(boot_device);

	return boot_device;
}

#ifdef CONFIG_BOOTCOUNT_LIMIT
unsigned long bootcount_load(void)
{
	return 0;
}

void bootcount_store(unsigned long bootcount)
{
}
#endif
