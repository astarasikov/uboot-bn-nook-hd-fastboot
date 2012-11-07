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

#ifndef _BQ2419x_H_
#define _BQ2419x_H_

#define WatchDog_40s 1
#define WatchDog_80s 2
#define WatchDog_160s 3
#define WatchDog_LSHIFT 4

/* Charging Status */
enum {
	NOT_CHARGING,
	PRE_CHARGING,
	FAST_CHARGING,
	CHARGE_DONE
};

/* VBus Status */
enum {
	VBUS_UNKNOWN,
	VBUS_USB_HOST,
	VBUS_ADAPTER_PORT,
	VBUS_OTG
};

int bq2419x_reset(void);
void bq2419x_init();
int bq2419x_get_charging_status(u8 *chrg_stat);
int bq2419x_get_vbus_status(u8 *vbus_stat);
int bq2419x_get_part_number(u8 *val);
int bq2419x_get_fault(u8 *val);
int bq2419x_select_power_source(u8 pwr_src);
int bq2419x_enable_dpdm(int enable);
int bq2419x_disable_watchdog(void);

#endif	/* _BQ2419x_H_ */
