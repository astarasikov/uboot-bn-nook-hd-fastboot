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

/*
 * Various B&N boot support functions shared between the different HW platforms
 */

#include <common.h>
#include <bn_boot.h>

struct img_info bootimg_info = {
	.width = 1920,
	.height = 1280,
	.bg_color = 0x0,
};

int display_mmc_gzip_ppm(int mmc, int part, const char *filename, uint32_t *fb, int width, int height)
{
	return 0;
}
