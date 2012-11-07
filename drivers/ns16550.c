/*
 * This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *  
 * COM1 NS16550 support
 * originally from linux source (arch/ppc/boot/ns16550.c)
 * modified to use CFG_ISA_MEM and new defines
 */

#include <config.h>
#include <exports.h>

#ifdef CFG_NS16550

#include <ns16550.h>

#define LCRVAL LCR_8N1					/* 8 data, 1 stop, no parity */
#define MCRVAL (MCR_DTR | MCR_RTS)			/* RTS/DTR */
#define FCRVAL (FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)	/* Clear & enable FIFOs */

#define LSR_DATA_INOUT (LSR_DR | LSR_TEMT | LSR_THRE)
#define LSR_DATA_EMPTY (LSR_TEMT | LSR_THRE)
#define ERRATA_I202_TIMEOUT 5

/*
 * Work Around for Errata i202 (3430 - 1.12, 3630 - 1.6)
 * The access to uart register after MDR1 Access
 * causes UART to corrupt data.
 *
 * Need a delay =
 * 5 L4 clock cycles + 5 UART functional clock cycle (@48MHz = ~0.2uS)
 * give 5 times as much
 *
 * uart_no : Should be a Zero Based Index Value always.
 */

void omap_uart_mdr1_errataset(NS16550_t com_port, unsigned char mdr1_val,
				unsigned char fcr_val)
{
	/* 10 retries, in this the FiFO's should get cleared */
	unsigned char timeout = ERRATA_I202_TIMEOUT;

	com_port->mdr1 = mdr1_val;
	udelay(1);
	com_port->fcr = fcr_val;

	/* Wait for FIFO to empty: when empty, RX bit is 0 and TX bits is 1. */
	while ((com_port->lsr & LSR_DATA_INOUT) != LSR_DATA_EMPTY) {
		timeout--;
		if (!timeout) {
			/* Should *never* happen. we warn and carry on */
			printf("Errata i202: timedout %x\n", com_port->lsr);
			break;
		}
		udelay(1);
	}
}

void NS16550_init(NS16550_t com_port, int baud_divisor)
{
	com_port->ier = 0x00;
#if defined(CONFIG_OMAP) && !defined(CONFIG_3430ZOOM2)
	com_port->mdr1 = 0x7;	/* mode select reset TL16C750*/
#endif

#if defined(CONFIG_3430ZOOM2)
	/* On Zoom2 board Set pre-scalar to 1
	 * CLKSEL is GND => MCR[7] is 1 => preslr is 4
	 * So change the prescl to 1
	 */
	com_port->lcr = 0xBF;
	com_port->fcr |= 0x10;
	com_port->mcr &= 0x7F;
#endif
	com_port->lcr = LCR_BKSE | LCRVAL;
	com_port->dll = baud_divisor & 0xff;
	com_port->dlm = (baud_divisor >> 8) & 0xff;
	com_port->lcr = LCRVAL;
	com_port->mcr = MCRVAL;

#if defined(CONFIG_OMAP) && !defined(CONFIG_3430ZOOM2)
#if defined(CONFIG_APTIX)
	/* 13 mode so Aptix 6MHz can hit 115200 */
	omap_uart_mdr1_errataset(com_port, 3, FCRVAL);
#else
	/* 16 is proper to hit 115200 with 48MHz */
	omap_uart_mdr1_errataset(com_port, 0, FCRVAL);
#endif
#endif
}

void NS16550_reinit (NS16550_t com_port, int baud_divisor)
{
	com_port->ier = 0x00;
	com_port->lcr = LCR_BKSE;
	com_port->dll = baud_divisor & 0xff;
	com_port->dlm = (baud_divisor >> 8) & 0xff;
	com_port->lcr = LCRVAL;
	com_port->mcr = MCRVAL;
	com_port->fcr = FCRVAL;
}

void NS16550_putc (NS16550_t com_port, char c)
{
	while ((com_port->lsr & LSR_THRE) == 0);
	com_port->thr = c;
}

char NS16550_getc (NS16550_t com_port)
{
	while ((com_port->lsr & LSR_DR) == 0) {
#ifdef CONFIG_USB_TTY
		extern void usbtty_poll(void);
		usbtty_poll();
#endif
	}
	return (com_port->rbr);
}

int NS16550_tstc (NS16550_t com_port)
{
	return ((com_port->lsr & LSR_DR) != 0);
}

#endif
