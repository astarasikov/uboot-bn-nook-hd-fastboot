/*
 * Copyright (c) 2010, The Android Open Source Project.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Neither the name of The Android Open Source Project nor the names
 *    of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <common.h>
#include <part_efi.h>
#include <mmc.h>
#include <fastboot.h>
#include <asm/arch/sys_info.h>
#include <bn_boot.h>

static const struct efi_partition_info partitions[] = {
	/* name       start_kb    size_kb */
	{ "xloader",       128,       128 }, /* xloader must start at 128 KB */
	{ "bootloader",      0,       256 },
	{ "recovery",        0,     15872 },
	{ "boot",      16*1024,   16*1024 },
	{ "rom",             0,   48*1024 },
	{ "bootdata",        0,   48*1024 },
	{ "factory",         0,  370*1024 },
	{ "system",          0,  612*1024 },
	{ "cache",           0,  426*1024 },
	{ "media",           0, 1024*1024 },
	{ "userdata",        0,         0 }, /* autosize to the end */
	{},
};

int mmc_slot = 1;

int fastboot_oem(const char *cmd)
{
	return -1;
}

void board_mmc_init(void)
{
}

int board_late_init(void)
{
	set_boot_mode();
	return SD_UIMAGE;
}

