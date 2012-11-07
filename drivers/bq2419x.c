/*
 * Copyright (C) 2012 Barnes & Noble, Inc.
 *
 * bq2419x charger driver for u-boot
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <common.h>
#include <asm/arch/gpio.h>
#include <i2c.h>
#include <bq2419x.h>

#define BQ2419X_I2C_ADDRESS		(0x6b)

#define BQ2419X_PWR_ON_CONFIG_REG	(0x01)
#define PWR_ON_CONFIG_REG_RST_SHIFT	7
#define PWR_ON_CONFIG_REG_RST_MASK	(0x1 << PWR_ON_CONFIG_REG_RST_SHIFT)
#define PWR_ON_CONFIG_REG_WDTRST_SHIFT	6
#define PWR_ON_CONFIG_REG_WDTRST_MASK	(0x1 << PWR_ON_CONFIG_REG_WDTRST_SHIFT)
#define PWR_ON_CONFIG_REG_SYSMIN_SHIFT	1
#define PWR_ON_CONFIG_REG_SYSMIN_3v2_MASK	(0x7 << PWR_ON_CONFIG_REG_SYSMIN_SHIFT)



#define BQ2519X_CHG_TERM_TMR_REG	(0x05)
#define CHG_TERM_TMR_WDOG_SHIFT		4
#define CHG_TERM_TMR_WDOG_MASK		(0x3 << CHG_TERM_TMR_WDOG_SHIFT)

#define BQ2519X_MISC_OP_CTRL_REG	(0x07)
#define MISC_OP_CTRL_REG_DPDM_SHIFT	7
#define MISC_OP_CTRL_REG_DPDM_MASK	(0x1 << MISC_OP_CTRL_REG_DPDM_SHIFT)

#define CHG_TERM_TMR_WDOG_SHIFT		4
#define CHG_TERM_TMR_WDOG_MASK		(0x3 << CHG_TERM_TMR_WDOG_SHIFT)

#define BQ2519X_MISC_OP_CTRL_REG	(0x07)
#define MISC_OP_CTRL_REG_DPDM_SHIFT	7
#define MISC_OP_CTRL_REG_DPDM_MASK	(0x1 << MISC_OP_CTRL_REG_DPDM_SHIFT)

#define BQ2419X_SYSTEM_STATUS_REG	(0x08)
#define SYSTEM_STATUS_CHRG_STAT_SHIFT	4
#define SYSTEM_STATUS_CHRG_STAT_MASK	(0x3 << SYSTEM_STATUS_CHRG_STAT_SHIFT)
#define SYSTEM_STATUS_VBUS_STAT_SHIFT	6
#define SYSTEM_STATUS_VBUS_STAT_MASK	(0x3 << SYSTEM_STATUS_VBUS_STAT_SHIFT)

#define BQ2419X_FAULT_REG		(0x09)

#define BQ2419X_VND_STS_REG		(0x0A)
#define VND_STS_PART_NUM_SHIFT		3
#define VND_STS_PART_NUM_MASK		(0x7 << VND_STS_PART_NUM_SHIFT)

#define BIT(x)	(1<<(x))

#define BQ2419x 			BIT(5)
#define BQ24195 			BQ2419x
#define BQ24196				(BQ2419x|BIT(3))
#define BQ24196_REV_1_2			(BQ24196|BIT(0))
#define BQ24196_REV_1_3			(BQ24196|BIT(1))
#define BQ24196_REV_1_4			(BQ24196|BIT(1)|BIT(2))
#define BQ24196_REV_1_2			(BQ24196|BIT(0))
#define BQ24196_REV_1_3			(BQ24196|BIT(1))
#define BQ24196_REV_1_4			(BQ24196|BIT(1)|BIT(2))
#define BQ2419x_PN_REV_BIT_MASK		0x3F

#define BQ2419x_MINSYSV_3v2 2
#define BQ2419x_MINSYSV_3v5 5

#ifndef BQ2419X_GPIO_CE
#error BQ2419X_GPIO_CE should be defined in board config file
#endif

static u8 bq2419x_revision;
static int boostback_fix_needed;
static int current_charge_enabled;

extern int bq27520_get_voltage(u16 *val);

int bq2419x_set_watchdog(int);
int bq2419x_reset_watchdog(void);
int bq2419x_set_minvsysv(u8);

static int bq2419x_get_pn_revision(u8 *val);

static int bq2419x_read_register(u8 reg, u8 *val)
{
	return i2c_read(BQ2419X_I2C_ADDRESS, reg, 1, val, 1);
}

static int bq2419x_write_register(u8 reg, u8 val)
{
	return i2c_write(BQ2419X_I2C_ADDRESS, reg, 1, &val, 1);
}

int bq2419x_reset(void)
{
	return bq2419x_write_register(BQ2419X_PWR_ON_CONFIG_REG, PWR_ON_CONFIG_REG_RST_MASK);
}

void bq2419x_init(void)
{
	gpio_write(BQ2419X_GPIO_CE, 1);
	gpio_write(BQ2419X_GPIO_PSEL, 1);
	current_charge_enabled = 0;

	bq2419x_reset();
	bq2419x_get_pn_revision(&bq2419x_revision);

	switch (bq2419x_revision) {
	case BQ24196_REV_1_3:
		printf("charger chip PG1.3\n");
		boostback_fix_needed = 1;
	break;
	case BQ24196_REV_1_4:
		printf("charger chip PG1.4\n");
		boostback_fix_needed = 0;
	break;
	case BQ24196_REV_1_2:
		printf("charger chip PG1.2\n");
		boostback_fix_needed = 1;
	break;
	default:
		printf("unsupported bq chip!\n");
	break;
	}

	/* Init charger to 500mA limit */
	gpio_write(BQ2419X_GPIO_CE, 0);

}

int bq2419x_get_pn_revision(u8 *val)
{
	u8 data;
	int err;

	err = bq2419x_read_register(BQ2419X_VND_STS_REG, &data);
	if (!err)
		*val = (data & BQ2419x_PN_REV_BIT_MASK);

	return err;
}

int bq2419x_has_fault(void)
{
	int err = 0;
	u8 val_fault;
	u8 watchdog_fault;
	u8 otg_fault;
	u8 chrg_fault;
	u8 bat_fault;
	u8 ntc_fault;

	err = bq2419x_read_register(BQ2419X_FAULT_REG, &val_fault);
	if (err) {
		printf("bq2419x: unable to get fault register");
		return err;
	}

	if (val_fault)
		printf("bq2419x: reg09: 0x%x\n", val_fault);

	watchdog_fault = val_fault & 0x80;
	otg_fault = val_fault & 0x40;
	chrg_fault = (val_fault >> 4) & 0x03;
	bat_fault = val_fault & 0x08;
	ntc_fault = val_fault & 0x07;

	switch (chrg_fault)
	{
	case 3:
		printf("charger fault: Charge (Safe) timer expiration\n");
		err = 1;
		break;
	case 2:
		printf("charger fault: Thermal shutdown\n");
		err = 1;
		break;
	case 1:
		printf("charger fault: Input fault (OVP or bad source)\n");
		break;
	case 0:
		break;
	default:
		printf("charger fault: %d\n",chrg_fault);
		break;
	}
	switch (ntc_fault)
	{
	case 6:
		printf("ntc fault: Hot\n");
		err = 1;
		break;
	case 5:
		printf("ntc fault: Cold\n");
		err = 1;
		break;
	case 0:
		break;
	default:
		printf("ntc fault: %d\n",ntc_fault);
		err = 1;
		break;
	}

	return err;
}

int bq2419x_get_charging_status(u8 *chrg_stat)
{
	u8 status;
	int err;
	err = bq2419x_read_register(BQ2419X_SYSTEM_STATUS_REG, &status);
	if (!err)
		*chrg_stat = (status & SYSTEM_STATUS_CHRG_STAT_MASK) >> SYSTEM_STATUS_CHRG_STAT_SHIFT;

	return err;
}

int bq2419x_get_vbus_status(u8 *vbus_stat)
{
	u8 status;
	int err;
	err = bq2419x_read_register(BQ2419X_SYSTEM_STATUS_REG, &status);
	if (!err)
		*vbus_stat = (status & SYSTEM_STATUS_VBUS_STAT_MASK) >> SYSTEM_STATUS_VBUS_STAT_SHIFT;

	return err;
}

void bq2419x_fix_boost_back(void)
{
	if (boostback_fix_needed) {
		u16 current_vbat = 0;

		bq2419x_reset_wdt(WatchDog_40s);

		bq27520_get_voltage(&current_vbat);	//50ms
		printf("VBAT: %d\n", current_vbat);

		//220ms
		if ((current_vbat >= 3400) && (current_vbat <= 3650)) {
			bq2419x_set_minvsysv(BQ2419x_MINSYSV_3v2);
		} else
			bq2419x_set_minvsysv(BQ2419x_MINSYSV_3v5);

		update_backlight_state();
	}
}

void bq2419x_reset_wdt(int timeout)
{
	bq2419x_set_watchdog(timeout);	//215ms
	update_backlight_state();
	bq2419x_reset_watchdog();				//180ms
	update_backlight_state();
}

void bq2419x_enable_charging(int wall_charger)
{
	if (!current_charge_enabled) {
		current_charge_enabled = 1;

		bq2419x_fix_boost_back();
		gpio_write(BQ2419X_GPIO_PSEL, (wall_charger ? 0 : 1));
		gpio_write(BQ2419X_GPIO_CE, 0);

		if (BQ24196_REV_1_2 < bq2419x_revision)
			bq2419x_enable_dpdm(1);

		update_backlight_state();
	} else
		bq2419x_reset_wdt(WatchDog_160s);
}

void bq2419x_disable_charging(void)
{
	if (current_charge_enabled) {
		gpio_write(BQ2419X_GPIO_CE, 1);
		current_charge_enabled = 0;
	}
}

int bq2419x_enable_dpdm(int enable)
{
	u8 status;
	int err;

	err = bq2419x_read_register(BQ2519X_MISC_OP_CTRL_REG, &status);
	if (err)
		return err;
	if (enable)
		status |= MISC_OP_CTRL_REG_DPDM_MASK;
	else
		status &= ~MISC_OP_CTRL_REG_DPDM_MASK;

	err = bq2419x_write_register(BQ2519X_MISC_OP_CTRL_REG, status);

	return err;
}

int bq2419x_set_watchdog(int time)
{
	u8 status;
	int err;

	err = bq2419x_read_register(BQ2519X_CHG_TERM_TMR_REG, &status);
	if (!err) {
		status &= ~CHG_TERM_TMR_WDOG_MASK;
		status |= (time << CHG_TERM_TMR_WDOG_SHIFT);
		err = bq2419x_write_register(BQ2519X_CHG_TERM_TMR_REG, status);
	}

	return err;
}

int bq2419x_reset_watchdog(void)
{
	u8 status;
	int err;

	err = bq2419x_read_register(BQ2419X_PWR_ON_CONFIG_REG, &status);
	if (!err) {
		status |= PWR_ON_CONFIG_REG_WDTRST_MASK;
		err = bq2419x_write_register(BQ2419X_PWR_ON_CONFIG_REG, status);
	}

	return err;
}

int bq2419x_set_minvsysv(u8 reg)
{
	u8 status;
	int err;

	err = bq2419x_read_register(BQ2419X_PWR_ON_CONFIG_REG, &status);
	if (!err) {
		status &= ~PWR_ON_CONFIG_REG_SYSMIN_3v2_MASK;
		status |= (reg << PWR_ON_CONFIG_REG_SYSMIN_SHIFT);
		err = bq2419x_write_register(BQ2419X_PWR_ON_CONFIG_REG, status);
	}

	return err;
}
