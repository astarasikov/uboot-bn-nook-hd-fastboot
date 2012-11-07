/*
 * Copyright (c) 2011, Barnes & Noble Inc.
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

#include <common.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <command.h>

#define GPIO_BANK_1_DATAOUT 0x4A31013C
#define GPIO_BANK_2_DATAOUT 0x4805513C
#define GPIO_BANK_3_DATAOUT 0x4805713C
#define GPIO_BANK_4_DATAOUT 0x4805913C
#define GPIO_BANK_5_DATAOUT 0x4805B13C
#define GPIO_BANK_6_DATAOUT 0x4805D13C

#define GPIO_BANK_1_DATAOE 0x4A310134
#define GPIO_BANK_2_DATAOE 0x48055134
#define GPIO_BANK_3_DATAOE 0x48057134
#define GPIO_BANK_4_DATAOE 0x48059134
#define GPIO_BANK_5_DATAOE 0x4805B134
#define GPIO_BANK_6_DATAOE 0x4805D134

#define GPIO_BANK_1_DATAIN 0x4A310138
#define GPIO_BANK_2_DATAIN 0x48055138
#define GPIO_BANK_3_DATAIN 0x48057138
#define GPIO_BANK_4_DATAIN 0x48059138
#define GPIO_BANK_5_DATAIN 0x4805B138
#define GPIO_BANK_6_DATAIN 0x4805D138

u8 gpio_read(int gpio)
{
	// begin ugly hack
	u32 oe_reg = 0;
	u32 in_reg = 0;
	u32 out_reg = 0;

	if (gpio < 32) {
		oe_reg = GPIO_BANK_1_DATAOE;
		in_reg = GPIO_BANK_1_DATAIN;
		out_reg = GPIO_BANK_1_DATAOUT;
	} else if (gpio < 64) {
		oe_reg = GPIO_BANK_2_DATAOE;
		in_reg = GPIO_BANK_2_DATAIN;
		out_reg = GPIO_BANK_2_DATAOUT;
	} else if (gpio < 96) {
		oe_reg = GPIO_BANK_3_DATAOE;
		in_reg = GPIO_BANK_3_DATAIN;
		out_reg = GPIO_BANK_3_DATAOUT;
	} else if (gpio < 128) {
		oe_reg = GPIO_BANK_4_DATAOE;
		in_reg = GPIO_BANK_4_DATAIN;
		out_reg = GPIO_BANK_4_DATAOUT;
	} else if (gpio < 160) {
		oe_reg = GPIO_BANK_5_DATAOE;
		in_reg = GPIO_BANK_5_DATAIN;
		out_reg = GPIO_BANK_5_DATAOUT;
	} else if (gpio < 192) {
		oe_reg = GPIO_BANK_6_DATAOE;
		in_reg = GPIO_BANK_6_DATAIN;
		out_reg = GPIO_BANK_6_DATAOUT;
	} else {
		printf("Unsupported GPIO %d!\n", gpio);
		return 0;
	}

	if (readl(oe_reg) & (1 << (gpio % 32))) {
		return !!(readl(in_reg) & (1 << (gpio % 32)));
	} else {
		return !!(readl(out_reg) & (1 << (gpio % 32)));
	}

}

void gpio_in(int gpio)
{
	if (gpio < 32) {
		sr32(GPIO_BANK_1_DATAOE, gpio, 1, 1);
	} else if (gpio < 64) {
		sr32(GPIO_BANK_2_DATAOE, gpio - 32, 1, 1);
	} else if (gpio < 96) {
		sr32(GPIO_BANK_3_DATAOE, gpio - 64, 1, 1);
	} else if (gpio < 128) {
		sr32(GPIO_BANK_4_DATAOE, gpio - 96, 1, 1);
	} else if (gpio < 160) {
		sr32(GPIO_BANK_5_DATAOE, gpio - 128, 1, 1);
	} else if (gpio < 192) {
		sr32(GPIO_BANK_6_DATAOE, gpio - 160, 1, 1);
	} else {
		printf("Unsupported GPIO %d!\n", gpio);
	}
}

void gpio_write(int gpio, u8 value)
{
	// begin ugly hack

	value = value ? 1 : 0;

	if (gpio < 32) {
		sr32(GPIO_BANK_1_DATAOE, gpio, 1, 0);
		sr32(GPIO_BANK_1_DATAOUT, gpio, 1, value);
		//printf("1 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_1_DATAOE), readl(GPIO_BANK_1_DATAOUT));
	} else if (gpio < 64) {
		sr32(GPIO_BANK_2_DATAOE, gpio - 32, 1, 0);
		sr32(GPIO_BANK_2_DATAOUT, gpio - 32, 1, value);
		//printf("2 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_2_DATAOE), readl(GPIO_BANK_2_DATAOUT));
	} else if (gpio < 96) {
		sr32(GPIO_BANK_3_DATAOE, gpio - 64, 1, 0);
		sr32(GPIO_BANK_3_DATAOUT, gpio - 64, 1, value);
		//printf("3 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_3_DATAOE), readl(GPIO_BANK_3_DATAOUT));
	} else if (gpio < 128) {
		sr32(GPIO_BANK_4_DATAOE, gpio - 96, 1, 0);
		sr32(GPIO_BANK_4_DATAOUT, gpio - 96, 1, value);
		//printf("4 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_4_DATAOE), readl(GPIO_BANK_4_DATAOUT));
	} else if (gpio < 160) {
		sr32(GPIO_BANK_5_DATAOE, gpio - 128, 1, 0);
		sr32(GPIO_BANK_5_DATAOUT, gpio - 128, 1, value);
		//printf("5 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_5_DATAOE), readl(GPIO_BANK_5_DATAOUT));
	} else if (gpio < 192) {
		sr32(GPIO_BANK_6_DATAOE, gpio - 160, 1, 0);
		sr32(GPIO_BANK_6_DATAOUT, gpio - 160, 1, value);
		//printf("6 oe 0x%08x out 0x%08x\n", readl(GPIO_BANK_6_DATAOE), readl(GPIO_BANK_6_DATAOUT));
	} else {
		printf("Unsupported GPIO %d!\n", gpio);
	}
}

int do_gpio(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
        int rcode = 0;
        ulong pin;

        switch (argc) {
        case 3:                 /* set pin */
                pin = simple_strtoul(argv[1], NULL, 10);

                if (strcmp(argv[2],"in") == 0) {
                        gpio_in(pin);
                } else if ((argv[2][0] == 'h') || (argv[2][0] == '1')) {
                        gpio_write(pin,  1);
                } else if ((argv[2][0] == 'l') || (argv[2][0] == '0')) {
                        gpio_write(pin,  0);
                } else {
                        printf("Usage:\n%s\n", cmdtp->usage);
                        printf("%s\n", cmdtp->help);
                        rcode = 1;
                        break;
                }
                /* FALL TROUGH */
        case 2:                 /* show pin status */
                pin = simple_strtoul(argv[1], NULL, 10);
                printf("%d\n", gpio_read(pin));
                return 0;
        default:
                printf("Usage:\n%s\n", cmdtp->usage);
                rcode = 1;
        }
        return rcode;
}

U_BOOT_CMD(
        gpio,   3,      1,      do_gpio,
        "gpio   - set/display gpio pins\n",
        "num [in|low|high|0|1]\n"
);


