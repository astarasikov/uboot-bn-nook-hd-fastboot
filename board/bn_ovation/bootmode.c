#include <common.h>
#include <part_efi.h>
#include <mmc.h>
#include <asm/arch/sys_info.h>
#include <twl6030.h>
#include <bn_boot.h>

static void set_boot_cmd( int boot_type)
{
	run_command("fastboot", 0);
	setenv("bootcmd", "fastboot;");
	setenv ("altbootcmd", "run bootcmd");
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
