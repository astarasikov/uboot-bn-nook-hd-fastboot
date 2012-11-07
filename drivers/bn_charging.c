/*
 * Copyright (c) 2012, Barnes & Noble Inc.
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

#ifdef BOARD_CHARGING

#include <twl6030.h>
#include <bq27520.h>
#include <bq2419x.h>
#include <asm/io.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/gpio.h>
#include <asm-arm/errno.h>
#include <bn_boot.h>

#ifndef BOOT_RECOVERY
#error BOOT_RECOVERY should be defined in board config file
#endif

#ifndef BOOT_ANDROID
#error BOOT_ANDROID should be defined in board config file
#endif

#ifndef BOOT_SWUPDATE
#error BOOT_SWUPDATE should be defined in board config file
#endif

#ifndef VBAT_THRESH_MIN
#error VBAT_THRESH_MIN should be defined in board config file
#endif

#ifndef BQ2419X_GPIO_PSEL
#error BQ2419X_GPIO_PSEL should be defined in board config file
#endif

#define BATTERY_INIT_RETRIES		3
#define BQ27520_DEVICE_TYPE		0x0520

#define OMAP4_CONTROL_USB2PHYCORE	0x4A100620
#define OMAP4_CONTROL_DEV_CONF		0x4A002300
#define CM_ALWON_USBPHY_CLKCTRL		0x4A008640

#define USB2PHY_CHG_DET_STATUS_MASK	(0x7 << 21)
#define USB2PHY_CHGDETECTED		(1 << 13)
#define USB2PHY_CHGDETDONE		(1 << 14)
#define USB2PHY_RESTARTCHGDET		(1 << 15)
#define USB2PHY_DISCHGDET		(1 << 30)
#define DELAY_TO_SHUTDOWN	200000

#define HOME_BUTTON	32
#define DISPLAY_TIMEOUT	3000

/* Max Battery Temp is 60C, but max lcd operating temp is 50C */
#define FATAL_BATTERY_TEMP 510
#define MAX_BATTERY_TEMP 485
#define MIN_BATTERY_TEMP -200
#define MAX_CHARGE_TEMP 450
#define MIN_CHARGE_TEMP 0

enum usb2_phy_charger_status_t {
	USB2PHY_WAIT_STATE =		0x0,
	USB2PHY_NO_CONTACT =		0x1,
	USB2PHY_PS2 =			0x2,
	USB2PHY_UNKNOWN_ERROR =		0x3,
	USB2PHY_DEDICATED_CHARGER =	0x4,
	USB2PHY_HOST_CHARGER =		0x5,
	USB2PHY_PC =			0x6,
	USB2PHY_INT =			0x7,
};

enum charge_level_t {
	CHARGE_DISABLE,
	CHARGE_100mA,
	CHARGE_500mA,
	CHARGE_2000mA,
};

int boot_cooldown_charger = 0;
enum coolcharger_ppz_t coolcharger_ppz = COOLCHARGER_DISABLE;
extern int bq2419x_enable_charging(int wall_charger);

int check_keydown_duration_ms = 100;
int can_show_image = 0;


static int is_key_pressed()
{
	u8 pwron = 0;
	if (twl6030_hw_status(&pwron)) {
		printf("Failed to read twl6030 hw_status\n");
		pwron = STS_PWRON;
	}
	return ((gpio_read(HOME_BUTTON) == 0) || (pwron & STS_PWRON) != STS_PWRON);
}

static ulong get_duration(ulong start)
{
	ulong now = get_timer(start);
	now = now / (CFG_HZ / 1000);
//	printf("duration:%d\n", now);
	return now;
}

void update_backlight_state()
{
	if(can_show_image == 0)
		return;

	static ulong display_start = 0;
	if(display_start == 0)
		display_start = get_timer(0);

	if(get_duration(display_start) > DISPLAY_TIMEOUT && panel_is_enabled())
	{
		printf("Timeout to turn display off\n");
		turn_panel_off();
	}
	if(is_key_pressed())
	{
		printf("KEY PRESSED!!!\n");
		turn_panel_on();
		display_start = get_timer(0);
	}

}

static enum charge_level_t omap4_charger_detect(void)
{
	unsigned long phy_core, phy_core_restartdet_low, phy_core_restartdet_high;
	enum charge_level_t ret;

	sr32(CM_L3INIT_USBPHY_CLKCTRL, 0, 32, 0x101);
	sr32(CM_ALWON_USBPHY_CLKCTRL, 0, 32, 0x100);

	twl6030_enable_ldousb(1);
	writel(0, OMAP4_CONTROL_DEV_CONF);
	phy_core = readl(OMAP4_CONTROL_USB2PHYCORE);
	writel(phy_core & ~USB2PHY_DISCHGDET, OMAP4_CONTROL_USB2PHYCORE);
//	udelay(2000*1000);
	int i;
	for(i = 0; i< 2000 / check_keydown_duration_ms ; i++)
	{
		update_backlight_state();
		udelay(check_keydown_duration_ms * 1000);
	}
	phy_core = readl(OMAP4_CONTROL_USB2PHYCORE);

	switch ((phy_core & USB2PHY_CHG_DET_STATUS_MASK) >> 21) {
	case USB2PHY_DEDICATED_CHARGER:
	case USB2PHY_HOST_CHARGER:
		ret = CHARGE_2000mA;
		break;

	case USB2PHY_PC:
		ret = CHARGE_500mA;
		break;

	case USB2PHY_WAIT_STATE:
	case USB2PHY_NO_CONTACT:
	case USB2PHY_PS2:
	case USB2PHY_UNKNOWN_ERROR:
	case USB2PHY_INT:
	default:
		printf("OMAP4_CONTROL_USB2PHYCORE: 0x%08x\n", phy_core);
		ret = CHARGE_DISABLE;
	}
	phy_core_restartdet_low  = (phy_core & ~USB2PHY_DISCHGDET) & ~USB2PHY_RESTARTCHGDET;
	phy_core_restartdet_high = (phy_core & ~USB2PHY_DISCHGDET) | USB2PHY_RESTARTCHGDET;

	writel(phy_core_restartdet_high, OMAP4_CONTROL_USB2PHYCORE);
	udelay(4*1000);

	writel(phy_core_restartdet_low, OMAP4_CONTROL_USB2PHYCORE);

	writel(1, OMAP4_CONTROL_DEV_CONF);
	twl6030_enable_ldousb(0);
	return ret;
}

static enum charge_level_t charger_detect(void)
{
	int ret;
	u8 hw_status;
	u8 vbus_status;

	ret = twl6030_hw_status(&hw_status);

	if (ret) {
		printf("Failed to read hw_status, reason: %d\n", ret);
		return CHARGE_DISABLE;
	}

	ret = twl6030_vbus_status(&vbus_status);

	if (ret) {
		printf("Failed to read vbus_status, reason: %d\n", ret);
		return CHARGE_DISABLE;
	}
	update_backlight_state();
	if ((vbus_status & VBUS_DET) && !(hw_status & STS_USB_ID)) {
		return omap4_charger_detect();
	} else {
		/* No charger detected */
		return CHARGE_DISABLE;
	}
}

static void charger_enable(enum charge_level_t charger)
{
	switch (charger) {
	case CHARGE_2000mA:
		bq2419x_enable_charging(1);
		break;
	case CHARGE_500mA:
		bq2419x_enable_charging(0);
		break;
	case CHARGE_DISABLE:
		bq2419x_disable_charging();
		break;
	default:
		break;
	}
}


static void print_vbus_status(u8 vbus_stat)
{
	switch (vbus_stat) {
	case VBUS_ADAPTER_PORT:
		printf("Adapter charger\n");
		break;
	case VBUS_USB_HOST:
		printf("PC charger\n");
		break;
	case VBUS_OTG:
		printf("OTG charger\n");
		break;
	case VBUS_UNKNOWN:
	default:
		printf("No charger\n");
	}
}

static int charge_loop(uint16_t *batt_soc)
{
	u16 soc, bat_vol;
	u8 vbus_stat, last_vbus_stat = -1;
	enum charge_level_t charger;
	int i, err;
	int count = 1;
	*batt_soc = 0;

	do {
		err = bq2419x_get_vbus_status(&vbus_stat);	//160ms
		if (err) {
			printf("charger: get vbus status failed\n");
			break;
		}
		if (vbus_stat != last_vbus_stat)
			print_vbus_status(vbus_stat);

		last_vbus_stat = vbus_stat;

		if (vbus_stat == VBUS_UNKNOWN) {
			printf("charger: no vbus supply\n");
			break;
		}

		err = bq2419x_has_fault();	//150ms
		if (err) {
			printf("charger: get fault status failed\n");
			break;
		}

		err = bq27520_get_soc(&soc);
		if (err) {
			printf("gg: get soc failed\n");
			break;
		}

		if (*batt_soc != soc)
			printf("%d\%\n", soc);
		else
		{
			printf("-");
			count++;
		}

		if(count % 70 == 0)
			printf("\n");

		*batt_soc = soc;

		/* delay is needed to prevent gauge lock up */
		for (i = 0; i < 3; i++) {
			charger = charger_detect();	//2.4s
			if (charger == CHARGE_DISABLE)
				break;
			can_show_image = charger > CHARGE_500mA ? 1 : 0;
			//udelay(500*1000);			//0.5s
			for(i = 0; i < 500 / check_keydown_duration_ms ; i++)
			{
				update_backlight_state();
				udelay(check_keydown_duration_ms * 1000);
			}
		}
		err = bq27520_get_voltage(&bat_vol);	//50ms
		if (err) {
			printf("gg: get voltage failed\n");
			break;
		}
		printf("VBAT: %d\n", bat_vol);

		charger_enable(charger);		//700ms
		update_backlight_state();

	} while (soc < BOOT_RECOVERY || bat_vol < VBAT_THRESH_MIN);

	if (err) {
		printf("charger reg dump:\n");
		run_command("imd 6b 0 b", 0);

		printf("\nGG reg dump:\n");
		run_command("imd 55 0 40", 0);
	}

	return err;
}

static int gas_gauge_check_bq27520(void)
{
	int err;
	u16 data;

	err = bq27520_get_control_device_type(&data);
	if (!err) {
		printf("gg: BQ27%X\n", data);
		if (data == BQ27520_DEVICE_TYPE) {
			err = 0;
		} else {
			printf("gg: wrong device type!\n");
			err = -ENODEV;
		}
	}

	return err;
}

static int gas_gauge_init(int retries)
{
	int err;

	err = bq27520_init(retries);

	switch (err) {
	case 0:
		err = gas_gauge_check_bq27520();
		if (!err)
			printf("gg: init done!\n");
		break;
	case -ENOBAT:
		err = gas_gauge_check_bq27520();
		if (!err) {
			printf("gg: no batt found!\n");
			err = -ENOBAT;
		}
		break;
	case -ETIME:
	default:
		printf("gg init failed\n");
		break;
	}

	return err;
}

static int charger_init(void)
{
	bq2419x_init();
	return 0;
}

int check_emmc_boot_mode(void);
static int check_is_sw_update()
{
	return check_emmc_boot_mode() == EMMC_RECOVERY;
}

int board_charging(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	enum charge_level_t charger = CHARGE_DISABLE;
	u16 tmp, batt_soc = 0;
	s16 batt_temp;
	u8 bat_det = 1;
	int ret;
	char command[512];

	if ((gd->bd->bi_arch_number != MACH_TYPE_OMAP4_HUMMINGBIRD ||
	     gd->bd->bi_board_revision < HUMMINGBIRD_EVT1) &&
	    (gd->bd->bi_arch_number != MACH_TYPE_OMAP4_OVATION ||
	     gd->bd->bi_board_revision < OVATION_EVT1B))
		goto EXIT;

	printf("** Checking Battery ***\n");

	ret = charger_init();
	if (ret) {
		printf("Failed to init charger\n");
		goto SHUTDOWN;
	}

	ret = gas_gauge_init(BATTERY_INIT_RETRIES);
	if (ret) {
		if (ret == -ENOBAT) {
			bat_det = 0;
		} else {
			printf("Failed to init gas gauge\n");
			goto SHUTDOWN;
		}
	}

	ret = bq27520_get_soc(&batt_soc);
	if (ret) {
		printf("Failed to get soc\n");
		goto REBOOT;
	}

	ret = bq27520_get_temperature(&tmp);
	if (ret) {
		printf("Failed to get battery temperature\n");
		goto SHUTDOWN;
	}

	/* temperature unit is 0.1K, convert to 0.1 deg Celsius */
	batt_temp = tmp - 2731;

	printf("Temperature: %d.%dC\n", batt_temp / 10, (batt_temp > 0?batt_temp:-batt_temp) % 10);

	if (batt_temp >= FATAL_BATTERY_TEMP || batt_temp <= MIN_BATTERY_TEMP) {
		printf("Battery temperature out of range\n");
		goto SHUTDOWN;
	}

	if( (batt_soc >= BOOT_ANDROID) && (batt_temp > MIN_BATTERY_TEMP && batt_temp < MAX_BATTERY_TEMP)){
        set_mpu_dpll_max_opp();
        show_image(boot);
	}

	printf("SOC: %d\%\n", batt_soc);

	ret = charger_init();
	if (ret) {
		printf("Failed to init charger\n");
		goto SHUTDOWN;
	}

	if (bat_det == 0) {
		charger = charger_detect();
		charger_enable(charger);
		if (charger <= CHARGE_500mA) {
			printf("Low input current! Not enough power to boot without battery.\n");
			charger_enable(CHARGE_DISABLE);
			goto SHUTDOWN;
		} else {
			printf("High input current source detected! Continue booting without battery.\n");
			charger_enable(CHARGE_DISABLE);
			goto EXIT;
		}
	}

BEGIN_CHECK_SOC:
	if (batt_soc < BOOT_RECOVERY) {
		charger = charger_detect();
		charger_enable(charger);
		if (charger == CHARGE_DISABLE) {
			printf("Battery is too low to boot, no display\n");
			//we don't turn on display if charge is too low.
			coolcharger_ppz = COOLCHARGER_SHUTDOWN_NOACTION;
			goto SHUTDOWN;
		}

		if (batt_temp >= MAX_CHARGE_TEMP) {
			printf("Battery is too hot to charge\n");
			//we only turn on the display when charger > 2000mA
			coolcharger_ppz = charger > CHARGE_500mA ? COOLCHARGER_SHUTDOWN_COOLDOWN : COOLCHARGER_SHUTDOWN_NOACTION;
			goto SHUTDOWN;
		}
		else if (batt_temp <= MIN_CHARGE_TEMP) {
			printf("Battery is too cold to charge\n");
			//we only turn on the display when charger > 2000mA
			coolcharger_ppz = charger > CHARGE_500mA ? COOLCHARGER_SHUTDOWN_WARMUP : COOLCHARGER_SHUTDOWN_NOACTION;
			goto SHUTDOWN;
		}

		printf("Low battery. Charge until %d\%\n", BOOT_RECOVERY);
		//we only turn on the display when charger > 2000mA
		if(charger > CHARGE_500mA) {
			show_image(lowbatt_charge);
			can_show_image = 1;
		}
		set_mpu_dpll_min_opp();
		ret = charge_loop(&batt_soc);
		if (ret)
			goto SHUTDOWN;

		if (batt_soc < BOOT_RECOVERY) {
			printf("Unable to charge to %d\%.\n", BOOT_RECOVERY);
			coolcharger_ppz = COOLCHARGER_SHUTDOWN_NOACTION;
			goto SHUTDOWN;
		}

		printf("Charge done, booting.\n");
	}

	if (get_boot_device(NULL) == BOOT_DEVICE_EMMC) {

		int boot_kernel_soc = BOOT_ANDROID;
		//we only check sw update when boot_device is EMMC, not for usb boot
		//otherwise the address 0x81000000 of ramdisk for usb boot will be overwritten by read_bcb().
		if(BOOT_ANDROID < BOOT_SWUPDATE && check_is_sw_update())
		{
			printf("SW update, charging to BOOT_SWUPDATE: %d\%\n", BOOT_SWUPDATE);
			boot_kernel_soc = BOOT_SWUPDATE;
		}

		if (batt_soc >= boot_kernel_soc) {
			/* Max LCD operating temp is 50C. So we should reboot at 51C (as reported by SW). */
			if (batt_temp >= MAX_BATTERY_TEMP) {
				printf("Hot battery, entering cooldown mode\n");
				sprintf(command, "setenv bootargs ${bootargs} androidboot.mode=cooldown batt_temp_reboot=470 batt_level_shutdown=%d", BOOT_RECOVERY);
				run_command(command, 0);
				coolcharger_ppz = COOLCHARGER_COOLDOWN_MODE;
				boot_cooldown_charger = 1;
			}
			goto EXIT;
		}

		/* if need to boot into coolcharger, set OPP50 to save more power */
		set_mpu_dpll_min_opp();

		charger = charger_detect();
		charger_enable(charger);

		if (charger != CHARGE_DISABLE) {
			//enter recovery mode to handle charger or cooldown
			//we don't show anything with usb charging until coolcharger starts.
			if(charger == CHARGE_500mA)
				coolcharger_ppz = COOLCHARGER_INITONLY;

			if (batt_temp >= 430) {
				printf("Battery is too hot to charge, entering cooldown mode\n");
				sprintf(command, "setenv bootargs ${bootargs} androidboot.mode=cooldown batt_temp_reboot=400 batt_level_shutdown=%d", BOOT_RECOVERY);
				run_command(command, 0);
				coolcharger_ppz = COOLCHARGER_COOLDOWN_MODE;
				boot_cooldown_charger = 1;
			} else if (batt_temp >= MIN_CHARGE_TEMP) {
				printf("Boot to charger mode and charge until %d\%\n", boot_kernel_soc);
				sprintf(command, "setenv bootargs ${bootargs} androidboot.mode=charger batt_level_shutdown=%d batt_level_android=%d", BOOT_RECOVERY, boot_kernel_soc + 1);
				run_command(command, 0);
				coolcharger_ppz = charger > CHARGE_500mA ? COOLCHARGER_CHARGER_MODE: COOLCHARGER_INITONLY;
				boot_cooldown_charger = 1;
			} else if (batt_temp < MIN_CHARGE_TEMP ){
				printf("Battery is too cold to charge\n");
				coolcharger_ppz = COOLCHARGER_SHUTDOWN_WARMUP;
				goto SHUTDOWN;
			}
		} else {
			//the charger is not connected.
			printf("Battery is too low to boot\n");
			show_image(connect_charge);
			//wait for 3 seconds to shutdown and check if charger is connected.
			int i = 0;
			for(i = 0; i < 3; i++) {
				printf("%d:Waiting for charger plugging...\n",i);
				udelay(1000000);

				charger = charger_detect();
				if(charger != CHARGE_DISABLE) {
					coolcharger_ppz = COOLCHARGER_DISABLE;
					charger_enable(charger);
					turn_panel_off();
					goto BEGIN_CHECK_SOC;
				}
			}
			//we already show connect_charge, so just shutdown here.
			coolcharger_ppz = COOLCHARGER_SHUTDOWN_NOACTION;
			goto SHUTDOWN;
		}
	}

EXIT:
	printf("End Charging in u-boot\n");
	set_mpu_dpll_max_opp();
	goto BOARD_CHARGING_RETURN;

SHUTDOWN:
	setenv("late_shutdown_reboot", "1");
	printf("Will shutdown after autoboot timer expires\n");
	goto BOARD_CHARGING_RETURN;

REBOOT:
	setenv("late_shutdown_reboot", "2");
	printf("Will reboot after autoboot timer expires\n");
	/* fall-thru */

BOARD_CHARGING_RETURN:
	/* Always break the boost back condition */
	bq2419x_fix_boost_back();
	return 0;
}

int boot_cooldown_charger_mode(void)
{
	return boot_cooldown_charger;
}

enum coolcharger_ppz_t get_coolcharger_ppz(void)
{
	return coolcharger_ppz;
}

#endif	/* BOARD_CHARGING */
