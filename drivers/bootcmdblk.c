/*
 * (C) Copyright 2012
 * Barnes & Noble, <www.bn.com>
 * Vadim Mikhailov <vmikhailov@book.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <common.h>
#include <bootcmdblk.h>
#include <asm/io.h>
#include <twl4030.h>

int bootcmdblk_get_cmd(char *message)
{
	unsigned char bootmode = BOOTCMDBLK_NORMAL;
#if defined(CONFIG_OMAP44XX)
	if (__raw_readl(PRM_RSTST) & PRM_RSTST_RESET_WARM_BIT) {
		if (message) {
			strncpy(message, PUBLIC_SAR_RAM_1_FREE, 16);
		}
		if (!strcmp(PUBLIC_SAR_RAM_1_FREE, "recovery"))
			bootmode = BOOTCMDBLK_RECOVERY;
		if (!strcmp(PUBLIC_SAR_RAM_1_FREE, "bootloader"))
			bootmode = BOOTCMDBLK_BOOTLOADER;
		if (!strcmp(PUBLIC_SAR_RAM_1_FREE, "off"))
			bootmode = BOOTCMDBLK_OFF;
		if (!strcmp(PUBLIC_SAR_RAM_1_FREE, "ignore"))
			bootmode = BOOTCMDBLK_IGNORE;
	}
#endif
#if defined(CONFIG_DRIVER_OMAP34XX_I2C)
	i2c_read(TWL4030_CHIP_BACKUP, TWL4030_BASEADD_BACKUP + 0,
		1, &bootmode, 1);
	if (message) {
		switch(bootmode) {
		case BOOTCMDBLK_RECOVERY:
			strcpy(message, "recovery");
			break;
		case BOOTCMDBLK_BOOTLOADER:
			strcpy(message, "bootloader");
			break;
		}
	}
#endif
#if defined(CFG_BOOTCMDBLK_ADDR)
	/* also handle bootcmd from scratchpad if present */
	if (message) {
		/* load bootcmd from scratchpad memory */
		memcpy((u_char *) message, CFG_BOOTCMDBLK_ADDR, BCB_COMMAND_SIZE);
		/* Ensure that the command message is NULL terminated */
		message[BCB_COMMAND_SIZE-1] = '\0';
		if (!strcmp("boot-recovery", message)) {
			strcpy(message, "recovery");
			bootmode = BOOTCMDBLK_RECOVERY;
		}
		if (!strcmp("recovery", message)) {
			bootmode = BOOTCMDBLK_RECOVERY;
		}
		if (!strcmp("bootloader", message)) {
			bootmode = BOOTCMDBLK_BOOTLOADER;
		}
	}
#endif
	return bootmode;
}

int bootcmdblk_set_cmd(char* message)
{
	unsigned char bootmode = 0;
	if (message)
		bootmode = message[0];
#if defined(CONFIG_OMAP44XX)
	if (message)
		strncpy(PUBLIC_SAR_RAM_1_FREE, message, 16);
#endif
#if defined(CONFIG_DRIVER_OMAP34XX_I2C)
	i2c_write(TWL4030_CHIP_BACKUP, TWL4030_BASEADD_BACKUP + 0,
		1, &bootmode, 1);
#endif
	return bootmode;
}
