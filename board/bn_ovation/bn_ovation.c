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
#include <omap4_dsi.h>
#include <lp855x.h>
#include <twl6030.h>

volatile int UBOOT_SEC_VER = 0;

void set_muxconf_regs(void)
{
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

#define GPIO_HW_ID0	33
#define GPIO_HW_ID1	34
#define GPIO_HW_ID2	35
u8 get_product_id(void)
{
	u8 hw_id0, hw_id1, hw_id2;

	hw_id0 = gpio_read(GPIO_HW_ID0);
	hw_id1 = gpio_read(GPIO_HW_ID1);
	hw_id2 = gpio_read(GPIO_HW_ID2);

	return ((hw_id0 << 2) + (hw_id1 << 1) + (hw_id2));
}

#define DDR_SIZE_512MB  0
#define DDR_SIZE_1GB    1
#define DDR_SIZE_2GB    2

#define GPIO_HW_ID3     40
#define GPIO_HW_ID4     41

u32 sdram_size_hwid(void)
{
	u8 hw_id4, hw_id3;

	hw_id4 = gpio_read(GPIO_HW_ID4);
	hw_id3 = gpio_read(GPIO_HW_ID3);

	switch ((hw_id3 << 1) | hw_id4) {
	case DDR_SIZE_512MB:
		return SZ_512M;
	case DDR_SIZE_1GB:
		return SZ_1G;
	case DDR_SIZE_2GB:
		return SZ_2G - SZ_4K;
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
	gd->bd->bi_arch_number = MACH_TYPE_OMAP4_OVATION;
	gd->bd->bi_boot_params = (0x80000000 + 0x100); /* boot param addr */
	gd->bd->bi_board_revision = identify_board_revision();
	return 0;
}

const char* board_rev_string(ulong btype)
{
    switch (btype) {
        case OVATION_EVT0:
            return "EVT0";
        case OVATION_EVT0B:
            return "EVT0B";
        case OVATION_EVT1:
            return "EVT1A";
        case OVATION_EVT1B:
            return "EVT1B";
        case OVATION_EVT2:
            return "EVT2";
        case OVATION_DVT:
            return "DVT";
        case OVATION_PVT:
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

/****************************************************************************
 * check_fpga_revision number: the rev number should be a or b
 ***************************************************************************/
inline u16 check_fpga_rev(void)
{
	return __raw_readw(FPGA_REV_REGISTER);
}

/****************************************************************************
 * check_uieeprom_avail: Check FPGA Availability
 * OnBoard DEBUG FPGA registers need to be ready for us to proceed
 * Required to retrieve the bootmode also.
 ***************************************************************************/
int check_uieeprom_avail(void)
{
	volatile unsigned short *ui_brd_name =
	    (volatile unsigned short *)EEPROM_UI_BRD + 8;
	int count = 1000;

	/* Check if UI revision Name is already updated.
	 * if this is not done, we wait a bit to give a chance
	 * to update. This is nice to do as the Main board FPGA
	 * gets a chance to know off all it's components and we can continue
	 * to work normally
	 * Currently taking 269* udelay(1000) to update this on poweron
	 * from flash!
	 */
	while ((*ui_brd_name == 0x00) && count) {
		udelay(200);
		count--;
	}
	/* Timed out count will be 0? */
	return count;
}

/***********************************************************************
 * get_board_type() - get board type based on current production stats.
 *  - NOTE-1-: 2 I2C EEPROMs will someday be populated with proper info.
 *    when they are available we can get info from there.  This should
 *    be correct of all known boards up until today.
 *  - NOTE-2- EEPROMs are populated but they are updated very slowly.  To
 *    avoid waiting on them we will use ES version of the chip to get info.
 *    A later version of the FPGA migth solve their speed issue.
 ************************************************************************/
u32 get_board_type(void)
{
	return SDP_4430_V1;
}

#define GPIO_LCD_PWR_EN		(36)
#define GPIO_LCD_BL_PWR_EN	(38)
#define GPIO_TP_RESET		(39)
#define GPIO_LCD_DCR_1V8	(27)
#define GPIO_LCD_CM_EN		(145)

void panel_enable(u8 onoff)
{
	gpio_write(GPIO_TP_RESET, 0);
	udelay(1000);

	// enable 1.8V first for touch
	twl6030_ldo_set_voltage(TWL6032_LDO_3, 1800);
	twl6030_ldo_set_trans(TWL6032_LDO_3, 1);
	twl6030_ldo_enable(TWL6032_LDO_3, 1);

	udelay(10*1000);

	gpio_write(GPIO_LCD_DCR_1V8, onoff);
	gpio_write(GPIO_LCD_CM_EN, onoff);
	udelay(10*1000);

	gpio_write(GPIO_LCD_PWR_EN, onoff);
	udelay(100*1000); // T2 timing
}

void backlight_enable(u8 onoff)
{
//	twl6030_ldo_set_voltage(TWL6032_LDO_3, 1800);
//	twl6030_ldo_set_trans(TWL6032_LDO_3, 1);
//	twl6030_ldo_enable(TWL6032_LDO_3, 1);

	gpio_write(GPIO_LCD_BL_PWR_EN, onoff);

	if (onoff) {
		udelay(100*1000);
		lp855x_on((0x1 << 4) | (0x4));
	}
}

void display_init(struct omap_dsi_panel *panel, void *fb, enum omap_dispc_format fmt)
{
	dss_init();
	dispc_init();
	dsi_init();

	udelay(10*1000);
	dsi_enable_panel(panel);
	dispc_config(panel, fb, fmt);

	dsi_enable_hs();
	dsi_turn_on_peripheral();
	dsi_send_null();

	dsi_enable_videomode(panel);
	dispc_go();
}

void backlight_set_brightness(u8 brightness)
{
	lp855x_set_brightness(brightness);
	// this is here due to buggy drivers not initializing
	// the i2c bus before they start banging away at it
	lp855x_restore_i2c();
}

