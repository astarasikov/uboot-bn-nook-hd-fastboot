/*
 * Copyright 2012 (c) Barnes & Noble Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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

#ifndef BN_BOOT_H
#define BN_BOOT_H

#define SD_UIMAGE        0
#define SD_BOOTIMG       1
#define EMMC_ANDROID     2
#define EMMC_RECOVERY    3
#define USB_BOOTIMG      4

enum coolcharger_ppz_t {
	COOLCHARGER_DISABLE,					//normal boot without coolcharger handling
	COOLCHARGER_SHUTDOWN_NOACTION,			//cannot show anything because of too low battery
	COOLCHARGER_SHUTDOWN_COOLDOWN,			//show cooldown.ppz
	COOLCHARGER_SHUTDOWN_WARMUP,			//show cooldown.ppz
	COOLCHARGER_INITONLY,					//booting with usb charging when dead battery, just init display
	COOLCHARGER_COOLDOWN_MODE,				//show cooldown.ppz if booting into cooldown mode
	COOLCHARGER_CHARGER_MODE,				//show charger.ppz if booting into charger mode
};

enum coolcharger_ppz_t get_coolcharger_ppz(void);

struct img_info {
	int 		width;
	int 		height;
	uint16_t 	bg_color;
};

int set_boot_mode(void);
void display_rle(uint16_t const *start, uint16_t *fb, int width, int height);
int display_ppm(uint8_t *ppm, uint32_t *fb, int width, int height);
int display_gzip_ppm(ppz_images use ,uint32_t *fb, int width, int height);
int display_mmc_gzip_ppm(int mmc, int part, const char *filename, uint32_t *fb, int width, int height);
int boot_display_gzip_ppm(uint32_t *fb, int width, int height);

void turn_panel_on(void);
void turn_panel_off(void);
int panel_is_enabled(void);
void update_backlight_state(void);
#endif /* BN_BOOT_H */
