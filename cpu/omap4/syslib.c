/*
 * (C) Copyright 2006-2009
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
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
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/arch/bits.h>
#include <asm/arch/mem.h>
#include <asm/arch/clocks.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>

/************************************************************
 * sdelay() - simple spin loop.  Will be constant time as
 *  its generally used in bypass conditions only.  This
 *  is necessary until timers are accessible.
 *
 *  not inline to increase chances its in cache when called
 *************************************************************/
void sdelay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
					  "bne 1b" : "=r" (loops) : "0"(loops));
}

/*****************************************************************
 * sr32 - clear & set a value in a bit range for a 32 bit address
 *****************************************************************/
void sr32(u32 addr, u32 start_bit, u32 num_bits, u32 value)
{
	u32 tmp, msk = 0;
	msk = 1 << num_bits;
	--msk;
	tmp = __raw_readl(addr) & ~(msk << start_bit);
	tmp |=  value << start_bit;
	__raw_writel(tmp, addr);
}

/*********************************************************************
 * wait_on_value() - common routine to allow waiting for changes in
 *   volatile regs.
 *********************************************************************/
u32 wait_on_value(u32 read_bit_mask, u32 match_value, u32 read_addr, u32 bound)
{
	u32 i = 0, val;
	do {
		++i;
		val = __raw_readl(read_addr) & read_bit_mask;
		if (val == match_value)
			return 1;
		if (i == bound)
			return 0;
	} while (1);
}

