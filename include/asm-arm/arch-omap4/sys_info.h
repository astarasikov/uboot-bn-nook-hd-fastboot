/*
 * (C) Copyright 2006-2009
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

#ifndef _OMAP44XX_SYS_INFO_H_
#define _OMAP44XX_SYS_INFO_H_

#define XDR_POP		5      /* package on package part */
#define SDR_DISCRETE	4      /* 128M memory SDR module*/
#define DDR_STACKED	3      /* stacked part on 2422 */
#define DDR_COMBO	2      /* combo part on cpu daughter card (menalaeus) */
#define DDR_DISCRETE	1      /* 2x16 parts on daughter card */

#define DDR_100		100    /* type found on most mem d-boards */
#define DDR_111		111    /* some combo parts */
#define DDR_133		133    /* most combo, some mem d-boards */
#define DDR_165		165    /* future parts */

#define CPU_4430	0x4430
#define CPU_4460	0x4460
#define CPU_4470	0x4470
#define CPU_UNKNOWN	0x0
#define OMAP44XX_CLASS	0x44

/*
 * omap_rev bits:
 * CPU id bits  (0730, 1510, 1710, 2422...)     [31:16]
 * CPU revision                                 [15:08]
 * CPU class bits (15xx, 16xx, 24xx, 34xx...)   [07:00]
 */
#define OMAP443X_CLASS		((CPU_4430 << 16) | OMAP44XX_CLASS)
#define OMAP4430_REV_ES1_0	(OMAP443X_CLASS | (0x00 << 8))
#define OMAP4430_REV_ES2_0	(OMAP443X_CLASS | (0x10 << 8))
#define OMAP4430_REV_ES2_1	(OMAP443X_CLASS | (0x20 << 8))
#define OMAP4430_REV_ES2_2	(OMAP443X_CLASS | (0x30 << 8))
#define OMAP4430_REV_ES2_3	(OMAP443X_CLASS | (0x40 << 8))
#define OMAP4430_REV_UNKNOWN	(OMAP443X_CLASS | (0xff << 8))

#define OMAP446X_CLASS		((CPU_4460 << 16) | OMAP44XX_CLASS)
#define OMAP4460_REV_ES1_0	(OMAP446X_CLASS | (0x10 << 8))
#define OMAP4460_REV_ES1_1	(OMAP446X_CLASS | (0x20 << 8))
#define OMAP4460_REV_UNKNOWN	(OMAP446X_CLASS | (0xff << 8))

#define OMAP447X_CLASS		((CPU_4470 << 16) | OMAP44XX_CLASS)
#define OMAP4470_REV_ES1_0	(OMAP447X_CLASS | (0x10 << 8))
#define OMAP4470_REV_UNKNOWN	(OMAP447X_CLASS | (0xff << 8))

/*
 * Omap device type
 */
#define OMAP_DEVICE_TYPE_TEST		0
#define OMAP_DEVICE_TYPE_EMU		1
#define OMAP_DEVICE_TYPE_SEC		2
#define OMAP_DEVICE_TYPE_GP		3
#define OMAP_DEVICE_TYPE_BAD		4

/* Currently Virtio models this one */
#define CPU_4430_CHIPID		0x0B68A000

#define GPMC_MUXED		1
#define GPMC_NONMUXED		0

#define TYPE_NAND		0x800	/* bit pos for nand in gpmc reg */
#define TYPE_NOR		0x000
#define TYPE_ONENAND		0x800

#define WIDTH_8BIT		0x0000
#define WIDTH_16BIT		0x1000	/* bit pos for 16 bit in gpmc */

#define I2C_MENELAUS		0x72	/* i2c id for companion chip */
#define I2C_TRITON2		0x4B	/* addres of power group */

#define BOOT_FAST_XIP		0x1f

/* SDP definitions according to FPGA Rev. Is this OK?? */
#define SDP_4430_VIRTIO		0x1
#define SDP_4430_V1		0x2

#define BOARD_4430_LABRADOR	0x80
#define BOARD_4430_LABRADOR_V1	0x1

#define BOARD_4430_ACCLAIM	0x3


#define PRODUCT_ACCLAIM		0x2
#define PRODUCT_OVATION		0x3
#define PRODUCT_HUMMINGBIRD	0x5

#define ACCLAIM_EVT0		0x0
#define ACCLAIM_EVT1A		0x1
#define ACCLAIM_EVT1B		0x2
#define ACCLAIM_EVT2		0x3
#define ACCLAIM_DVT		0x4
#define ACCLAIM_PVT		0x5
#define ELATION_EVT0		0x5

#define BOOT_DEVICE_SD		0x05
#define BOOT_DEVICE_EMMC	0x06
#define BOOT_DEVICE_USB		0x45

#define OVATION_EVT0		0x0
#define OVATION_EVT0B		0x1
#define OVATION_EVT0C		0x2
#define OVATION_EVT1		0x3
#define OVATION_EVT1B		0x4
#define OVATION_EVT2		0x5
#define OVATION_DVT		0x6
#define OVATION_PVT		0x7

#define HUMMINGBIRD_EVT0	0x0
#define HUMMINGBIRD_EVT0B	0x1
#define HUMMINGBIRD_EVT1	0x2
#define HUMMINGBIRD_EVT2	0x3
#define HUMMINGBIRD_DVT 	0x4
#define HUMMINGBIRD_PVT 	0x5

#endif
