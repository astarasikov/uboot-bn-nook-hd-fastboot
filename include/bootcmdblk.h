/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * sebastien griffoul <s-griffoul@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation's version 2 of
 * the License.
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

#define BCB_COMMAND_SIZE 32

typedef enum
{
	BOOTCMDBLK_NORMAL     = 0,
	BOOTCMDBLK_BOOTLOADER = 'b',
	BOOTCMDBLK_RECOVERY   = 'r',
	BOOTCMDBLK_OFF        = 'o',
	BOOTCMDBLK_IGNORE     = 'i',

}	bootcmdblk_cmd;

extern int bootcmdblk_get_cmd(char* message);
extern int bootcmdblk_set_cmd(char* message);
