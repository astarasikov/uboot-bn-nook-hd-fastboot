/*
 * Copyright (C) 2012 Barnes & Noble, Inc.
 *
 * bq27530 Gas Gauge initialization for u-boot
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
#include <bq27520.h>
#include <i2c.h>
#include <asm/arch/gpio.h>
#include <asm-arm/errno.h>

//#define BQ27520_DEBUG				1

#define BQ27520_I2C_ADDRESS			(0x55)
#define BQ27520_I2C_BUS_NUM			1

#define BQ27520_REG_CONTROL			(0x00)
#define BQ27520_CONTROL_CONTROL_STATUS		(0x0000)
#define BQ27520_CTRL_ST_INITCOMP_MASK		(1 << 7)
#define BQ27520_CONTROL_DEVICE_TYPE		(0x0001)
#define BQ27520_CONTROL_RESET			(0x0041)

#define BQ27520_REG_TEMPERATURE			(0x06)
#define BQ27520_REG_VOLTAGE			(0x08)
#define BQ27520_REG_FLAGS			(0x0a)
#define BQ27520_REG_FLAGS_BAT_DET_MASK		(1 << 3)
#define BQ27520_REG_STATE_OF_CHARGE		(0x2C)

#ifndef BQ27500_CE_GPIO
#error BQ27500_CE_GPIO should be defined in board config file
#endif

/* Start-up wait time for GG to have good data (tens of a second) */
#define BQ27520_STARTUP_TIME	10

#ifdef BQ27520_DEBUG
#define BQ_GG_DBG(x)	x
#else
#define BQ_GG_DBG(x)
#endif

static void bq27520_reset(void);

static int bq27520_reg_read(u16 reg, u16 *val)
{
	int err;
	int i;

	/* retry for 3 times */
	for (i = 0; i < 3; i++) {
		err = i2c_read_2_byte(BQ27520_I2C_ADDRESS, reg, (u8 *)val);
		if (err) {
			BQ_GG_DBG(printf("bq27520: register read failed. Trying to recovery with reset.\n"));
			bq27520_reset();
			err = bq27520_init(3);
			if (err && (err != -ENOBAT))
				break;
		} else
			break;
	}

	return err;
}

static int bq27520_i2c_control_read(u16 ctrl_cmd, u16 *data)
{
	uchar buffer[2];
	int err;

	buffer[0] = (ctrl_cmd & 0x00ff);
	buffer[1] = (ctrl_cmd & 0xff00)>>8;
	err = i2c_write(BQ27520_I2C_ADDRESS, BQ27520_REG_CONTROL, 1, buffer, sizeof(buffer));
	if (!err)
		err = bq27520_reg_read(BQ27520_REG_CONTROL, data);

	return err;
}

static int bq27520_control_write(u16 ctrl_cmd)
{
	uchar buffer[2];

	buffer[0] = (ctrl_cmd & 0x00ff);
	buffer[1] = (ctrl_cmd & 0xff00)>>8;

	return i2c_write(BQ27520_I2C_ADDRESS, BQ27520_REG_CONTROL, 1, buffer, sizeof(buffer));
}

static void bq27520_reset(void)
{
	BQ_GG_DBG(printf("bq27520 reseting...\n"));

	gpio_write(BQ27500_CE_GPIO, 1);
	udelay(200*1000);

	gpio_write(BQ27500_CE_GPIO, 0);
	udelay(3000*1000);

	BQ_GG_DBG(printf("bq27520 reset done\n"));
}

static int bq27520_get_initcomp(u8 *initcomp)
{
	int err;
	u16 status_reg;
	uchar buffer[2];

	buffer[0] = (BQ27520_CONTROL_CONTROL_STATUS & 0x00ff);
	buffer[1] = (BQ27520_CONTROL_CONTROL_STATUS & 0xff00) >> 8;

	err = i2c_write(BQ27520_I2C_ADDRESS, BQ27520_REG_CONTROL, 1, buffer, sizeof(buffer));
	if (!err) {
		err = i2c_read_2_byte(BQ27520_I2C_ADDRESS, BQ27520_REG_CONTROL, (u8 *)&status_reg);
		if (!err) {
			if (status_reg & BQ27520_CTRL_ST_INITCOMP_MASK)
				*initcomp = 1;
			else
				*initcomp = 0;
		} else
			BQ_GG_DBG(printf("bq27520: Error reading control_status!\n"));
	} else
		BQ_GG_DBG(printf("bq27520: Error reading control_status!\n"));

	return err;
}

static int bq27520_ready(int timeout)
{
	int err, i;
	u8 initcomp;

	for (i = 0; i < timeout; i++) {
		err = bq27520_get_initcomp(&initcomp);
		if (!err) {
			if (initcomp) {
				printf("\n");
				BQ_GG_DBG(printf("bq27520 Ready!\n"));
				return 0;
			} else {
				BQ_GG_DBG(printf("bq27520 Not ready! Waiting to become ready\n"));
			}
		} else {
			printf("\n");
			BQ_GG_DBG(printf("bq27520: Error reading INITCOM bit!\n"));
			return err;
		}

		printf("-");
		/* Polling the INITCOMP bit no more than every 500ms */
		udelay(500*1000);
	}

	printf("\n");
	BQ_GG_DBG(printf("bq27520: Timeout!\n"));

	return -ETIME;
}

int bq27520_init(int retries)
{
	int err;
	int i;
	u8 batt_flag;

	if (retries < 1) {
		printf("bq27520: Invalid input argument! retries = %d\n", retries);
		return -EINVAL;
	}

	gpio_write(BQ27500_CE_GPIO, 0);

	for (i = 0; i < retries; i++) {
		err = bq27520_ready(BQ27520_STARTUP_TIME);
		if (!err) {
			BQ_GG_DBG(printf("bq27520 gas gauge init done!\n"));
			break;
		} else {
			if (err == -ETIME) {
				err = bq27520_battery_detect(&batt_flag);
				if (err) {
					printf("bq27520: Failed to detect battery\n");
					break;
				}

				if (batt_flag) {
					BQ_GG_DBG(printf("bq27520: Battery detected!\n"));
					err = -ETIME;
					break;
				} else {
					BQ_GG_DBG(printf("bq27520: Battery not detected!\n"));
					err = -ENOBAT;
					break;
				}
			} else {
				if (i == (retries - 1)) {
					printf("bq27520 init failed! Give-up after %d retries\n", retries);
					break;
				}

				printf("bq27520 init failed, ERROR = %d. Reseting...\n", err);
 				bq27520_reset();
			}
		}
	}

	return err;
}

int bq27520_get_control_control_status(u16 *val)
{
	return bq27520_i2c_control_read(BQ27520_CONTROL_CONTROL_STATUS, val);
}

int bq27520_get_control_device_type(u16 *val)
{
	return bq27520_i2c_control_read(BQ27520_CONTROL_DEVICE_TYPE, val);
}

int bq27520_get_temperature(u16 *val)
{
	return bq27520_reg_read(BQ27520_REG_TEMPERATURE, val);
}

int bq27520_get_voltage(u16 *val)
{
	return bq27520_reg_read(BQ27520_REG_VOLTAGE, val);
}

int bq27520_get_soc(u16 *val)
{
	return bq27520_reg_read(BQ27520_REG_STATE_OF_CHARGE, val);
}

int bq27520_get_flags(u16 *val)
{
	return bq27520_reg_read(BQ27520_REG_FLAGS, val);
}

int bq27520_battery_detect(u8 *flag)
{
	u16 status;
	int err;

	err = bq27520_get_flags(&status);
	if (err)
		return err;

	*flag = status & BQ27520_REG_FLAGS_BAT_DET_MASK ? 1 : 0;

	return 0;
}

