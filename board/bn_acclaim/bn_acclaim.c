/*
 * (C) Copyright 2004-2009
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <asm/arch/mux.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/gpio.h>

int mmc_slot = 1;

#define		OMAP44XX_WKUP_CTRL_BASE		0x4A31E000
#if 1
#define M0_SAFE M0
#define M1_SAFE M1
#define M2_SAFE M2
#define M4_SAFE M4
#define M7_SAFE M7
#define M3_SAFE M3
#define M5_SAFE M5
#define M6_SAFE M6
#else
#define M0_SAFE M7
#define M1_SAFE M7
#define M2_SAFE M7
#define M4_SAFE M7
#define M7_SAFE M7
#define M3_SAFE M7
#define M5_SAFE M7
#define M6_SAFE M7
#endif
#define		MV(OFFSET, VALUE)\
			__raw_writew((VALUE), OMAP44XX_CTRL_BASE + (OFFSET));
#define		MV1(OFFSET, VALUE)\
			__raw_writew((VALUE), OMAP44XX_WKUP_CTRL_BASE + (OFFSET));

#define		CP(x)	(CONTROL_PADCONF_##x)
#define		WK(x)	(CONTROL_WKUP_##x)


/**********************************************************
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers
 *              specific to the hardware. Many pins need
 *              to be moved from protect to primary mode.
 *********************************************************/
void set_muxconf_regs(void)
{
	return;
}

/******************************************************************************
 * Routine: update_mux()
 * Description:Update balls which are different between boards.  All should be
 *             updated to match functionality.  However, I'm only updating ones
 *             which I'll be using for now.  When power comes into play they
 *             all need updating.
 *****************************************************************************/
void update_mux(u32 btype, u32 mtype)
{
	/* REVISIT  */
	return;

}

#ifdef CONFIG_BOARD_REVISION
#define GPIO_HW_ID5	49
#define GPIO_HW_ID6	50
#define GPIO_HW_ID7	51

static inline ulong identify_board_revision(void)
{
	u8 hw_id5, hw_id6, hw_id7;

	hw_id5 = gpio_read(GPIO_HW_ID5);
	hw_id6 = gpio_read(GPIO_HW_ID6);
	hw_id7 = gpio_read(GPIO_HW_ID7);

	return ((hw_id5 << 2) + (hw_id6 << 1) + (hw_id7));
}
#endif

#define DDR_SIZE_256MB  0
#define DDR_SIZE_512MB  1
#define DDR_SIZE_1GB    2

#define GPIO_HW_ID3     40
#define GPIO_HW_ID4     41

u32 sdram_size_hwid(void)
{
	u8 hw_id4, hw_id3;

	hw_id4 = gpio_read(GPIO_HW_ID4);
	hw_id3 = gpio_read(GPIO_HW_ID3);

	switch ((hw_id3 << 1) | hw_id4) {
	case DDR_SIZE_256MB:
		return SZ_256M;
	case DDR_SIZE_512MB:
		return SZ_512M;
	case DDR_SIZE_1GB:
		return SZ_1G;
	default:
		/* return 512M as safe value */
		return SZ_512M;
	}
}

/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gpmc_init();

#if 0 /* No eMMC env partition for now */
	/* Intializing env functional pointers with eMMC */
	boot_env_get_char_spec = mmc_env_get_char_spec;
	boot_env_init = mmc_env_init;
	boot_saveenv = mmc_saveenv;
	boot_env_relocate_spec = mmc_env_relocate_spec;
	env_ptr = (env_t *) (CFG_FLASH_BASE + CFG_ENV_OFFSET);
	env_name_spec = mmc_env_name_spec;
#endif

	/* board id for Linux */
#if defined(OMAP44XX_TABLET_CONFIG)
	gd->bd->bi_arch_number = MACH_TYPE_OMAP_BLAZE;
#else
	MV(CP(GPMC_NBE1), (M3)); /* gpio_60 */
	MV(CP(HDMI_HPD), ( M3));  /* gpio_63 */ 
	MV(CP(UNIPRO_TX1), ( M3 ));      /* gpio_173 */

	gd->bd->bi_arch_number = MACH_TYPE_OMAP4_ACCLAIM;
#endif
	gd->bd->bi_boot_params = (0x80000000 + 0x100); /* boot param addr */
	gd->bd->bi_board_revision = identify_board_revision();

	return 0;
}

const char* board_rev_string(ulong btype)
{
    switch (btype) {
        case ACCLAIM_EVT1A:
            return "EVT1A";
        case ACCLAIM_EVT1B:
            return "EVT1B";
        case ACCLAIM_EVT2:
            return "EVT2";
        case ACCLAIM_DVT:
            return "DVT";
        case ACCLAIM_PVT:
            return "PVT";
        default:
            return "Unknown";
    }
}

ulong get_board_revision(void)
{
	DECLARE_GLOBAL_DATA_PTR;
	return gd->bd->bi_board_revision;
}

#ifdef CONFIG_SERIAL_TAG
/*******************************************************************
 * get_board_serial() - Return board serial number
 * The default boot script must read the serial number from the ROM
 * tokenand store it in the "serialnum" environment variable.
 *******************************************************************/
void get_board_serial(struct tag_serialnr *serialnr)
{
	const char *sn = getenv("serialnum");
	if (sn) {
		/* Treat serial number as BCD */
		unsigned long long snint = simple_strtoull(sn, NULL, 16);
		serialnr->low = snint % 0x100000000ULL;
		serialnr->high = snint / 0x100000000ULL;
	} else {
		serialnr->low = 0;
		serialnr->high = 0;
	}
}

#endif

u32 get_board_type(void)
{
	return 0;
}

int board_late_init(void)
{
	printf("Board revision %s\n", board_rev_string(get_board_revision()));

	return determine_boot_type();
}

#define BOOT_TYPE ( ( *(volatile unsigned int*) ( 0x4A326000 ) ) & 0xFF )
#define SD_BOOT  (5)

int running_from_sd(void)
{
	unsigned int boot_type = BOOT_TYPE;
	return (boot_type == SD_BOOT);
}

