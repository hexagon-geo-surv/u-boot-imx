// SPDX-License-Identifier: GPL-2.0+
/*
 * (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <watchdog.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/clock.h>
#include <serial.h>
#include <linux/compiler.h>

#include "board_serial.h"

/* UART Control Register Bit Fields.*/
#define URXD_CHARRDY	(1<<15)
#define URXD_ERR	(1<<14)
#define URXD_OVRRUN	(1<<13)
#define URXD_FRMERR	(1<<12)
#define URXD_BRK	(1<<11)
#define URXD_PRERR	(1<<10)
#define URXD_RX_DATA	(0xFF)
#define UCR1_ADEN	(1<<15) /* Auto dectect interrupt */
#define UCR1_ADBR	(1<<14) /* Auto detect baud rate */
#define UCR1_TRDYEN	(1<<13) /* Transmitter ready interrupt enable */
#define UCR1_IDEN	(1<<12) /* Idle condition interrupt */
#define UCR1_RRDYEN	(1<<9)	/* Recv ready interrupt enable */
#define UCR1_RDMAEN	(1<<8)	/* Recv ready DMA enable */
#define UCR1_IREN	(1<<7)	/* Infrared interface enable */
#define UCR1_TXMPTYEN	(1<<6)	/* Transimitter empty interrupt enable */
#define UCR1_RTSDEN	(1<<5)	/* RTS delta interrupt enable */
#define UCR1_SNDBRK	(1<<4)	/* Send break */
#define UCR1_TDMAEN	(1<<3)	/* Transmitter ready DMA enable */
#define UCR1_UARTCLKEN	(1<<2)	/* UART clock enabled */
#define UCR1_DOZE	(1<<1)	/* Doze */
#define UCR1_UARTEN	(1<<0)	/* UART enabled */
#define UCR2_ESCI	(1<<15) /* Escape seq interrupt enable */
#define UCR2_IRTS	(1<<14) /* Ignore RTS pin */
#define UCR2_CTSC	(1<<13) /* CTS pin control */
#define UCR2_CTS	(1<<12) /* Clear to send */
#define UCR2_ESCEN	(1<<11) /* Escape enable */
#define UCR2_PREN	(1<<8)  /* Parity enable */
#define UCR2_PROE	(1<<7)  /* Parity odd/even */
#define UCR2_STPB	(1<<6)	/* Stop */
#define UCR2_WS		(1<<5)	/* Word size */
#define UCR2_RTSEN	(1<<4)	/* Request to send interrupt enable */
#define UCR2_TXEN	(1<<2)	/* Transmitter enabled */
#define UCR2_RXEN	(1<<1)	/* Receiver enabled */
#define UCR2_SRST	(1<<0)	/* SW reset */
#define UCR3_DTREN	(1<<13) /* DTR interrupt enable */
#define UCR3_PARERREN	(1<<12) /* Parity enable */
#define UCR3_FRAERREN	(1<<11) /* Frame error interrupt enable */
#define UCR3_DSR	(1<<10) /* Data set ready */
#define UCR3_DCD	(1<<9)  /* Data carrier detect */
#define UCR3_RI		(1<<8)  /* Ring indicator */
#define UCR3_ADNIMP	(1<<7)  /* Autobaud Detection Not Improved */
#define UCR3_RXDSEN	(1<<6)  /* Receive status interrupt enable */
#define UCR3_AIRINTEN	(1<<5)  /* Async IR wake interrupt enable */
#define UCR3_AWAKEN	(1<<4)  /* Async wake interrupt enable */
#define UCR3_REF25	(1<<3)  /* Ref freq 25 MHz */
#define UCR3_REF30	(1<<2)  /* Ref Freq 30 MHz */
#define UCR3_INVT	(1<<1)  /* Inverted Infrared transmission */
#define UCR3_BPEN	(1<<0)  /* Preset registers enable */
#define UCR4_CTSTL_32	(32<<10) /* CTS trigger level (32 chars) */
#define UCR4_INVR	(1<<9)  /* Inverted infrared reception */
#define UCR4_ENIRI	(1<<8)  /* Serial infrared interrupt enable */
#define UCR4_WKEN	(1<<7)  /* Wake interrupt enable */
#define UCR4_REF16	(1<<6)  /* Ref freq 16 MHz */
#define UCR4_IRSC	(1<<5)  /* IR special case */
#define UCR4_TCEN	(1<<3)  /* Transmit complete interrupt enable */
#define UCR4_BKEN	(1<<2)  /* Break condition interrupt enable */
#define UCR4_OREN	(1<<1)  /* Receiver overrun interrupt enable */
#define UCR4_DREN	(1<<0)  /* Recv data ready interrupt enable */
#define UFCR_RXTL_SHF	0       /* Receiver trigger level shift */
#define UFCR_RFDIV	(7<<7)  /* Reference freq divider mask */
#define UFCR_RFDIV_SHF	7	/* Reference freq divider shift */
#define RFDIV		4	/* divide input clock by 2 */
#define UFCR_DCEDTE	(1<<6)  /* DTE mode select */
#define UFCR_TXTL_SHF	10      /* Transmitter trigger level shift */
#define USR1_PARITYERR	(1<<15) /* Parity error interrupt flag */
#define USR1_RTSS	(1<<14) /* RTS pin status */
#define USR1_TRDY	(1<<13) /* Transmitter ready interrupt/dma flag */
#define USR1_RTSD	(1<<12) /* RTS delta */
#define USR1_ESCF	(1<<11) /* Escape seq interrupt flag */
#define USR1_FRAMERR	(1<<10) /* Frame error interrupt flag */
#define USR1_RRDY	(1<<9)	/* Receiver ready interrupt/dma flag */
#define USR1_TIMEOUT	(1<<7)	/* Receive timeout interrupt status */
#define USR1_RXDS	(1<<6)	/* Receiver idle interrupt flag */
#define USR1_AIRINT	(1<<5)	/* Async IR wake interrupt flag */
#define USR1_AWAKE	(1<<4)	/* Aysnc wake interrupt flag */
#define USR2_ADET	(1<<15) /* Auto baud rate detect complete */
#define USR2_TXFE	(1<<14) /* Transmit buffer FIFO empty */
#define USR2_DTRF	(1<<13) /* DTR edge interrupt flag */
#define USR2_IDLE	(1<<12) /* Idle condition */
#define USR2_IRINT	(1<<8)	/* Serial infrared interrupt flag */
#define USR2_WAKE	(1<<7)	/* Wake */
#define USR2_RTSF	(1<<4)	/* RTS edge interrupt flag */
#define USR2_TXDC	(1<<3)	/* Transmitter complete */
#define USR2_BRCD	(1<<2)	/* Break condition */
#define USR2_ORE	(1<<1)	/* Overrun error */
#define USR2_RDR	(1<<0)	/* Recv data ready */
#define UTS_FRCPERR	(1<<13) /* Force parity error */
#define UTS_LOOP	(1<<12) /* Loop tx and rx */
#define UTS_TXEMPTY	(1<<6)	/* TxFIFO empty */
#define UTS_RXEMPTY	(1<<5)	/* RxFIFO empty */
#define UTS_TXFULL	(1<<4)	/* TxFIFO full */
#define UTS_RXFULL	(1<<3)	/* RxFIFO full */
#define UTS_SOFTRS	(1<<0)	/* Software reset */
#define TXTL		2  /* reset default */
#define RXTL		1  /* reset default */

struct mxc_uart {
	u32 rxd;
	u32 spare0[15];

	u32 txd;
	u32 spare1[15];

	u32 cr1;
	u32 cr2;
	u32 cr3;
	u32 cr4;

	u32 fcr;
	u32 sr1;
	u32 sr2;
	u32 esc;

	u32 tim;
	u32 bir;
	u32 bmr;
	u32 brc;

	u32 onems;
	u32 ts;
};

static void _mxc_serial_init(struct mxc_uart *base, int use_dte)
{
	writel(0, &base->cr1);
	writel(0, &base->cr2);

	while (!(readl(&base->cr2) & UCR2_SRST));

	if (use_dte)
		writel(0x404 | UCR3_ADNIMP, &base->cr3);
	else
		writel(0x704 | UCR3_ADNIMP, &base->cr3);

	writel(0x704 | UCR3_ADNIMP, &base->cr3);
	writel(0x8000, &base->cr4);
	writel(0x2b, &base->esc);
	writel(0, &base->tim);

	writel(0, &base->ts);
}

static void _mxc_serial_setbrg(struct mxc_uart *base, unsigned long clk,
			       unsigned long baudrate, bool use_dte)
{
	u32 tmp;

	tmp = RFDIV << UFCR_RFDIV_SHF;
	if (use_dte)
		tmp |= UFCR_DCEDTE;
	else
		tmp |= (TXTL << UFCR_TXTL_SHF) | (RXTL << UFCR_RXTL_SHF);
	writel(tmp, &base->fcr);

	writel(0xf, &base->bir);
	writel(clk / (2 * baudrate), &base->bmr);

	writel(UCR2_WS | UCR2_IRTS | UCR2_RXEN | UCR2_TXEN | UCR2_SRST,
	       &base->cr2);
	writel(UCR1_UARTEN, &base->cr1);
}

static struct mxc_uart *mxc_base;

static void mxc_serial_setbrg(unsigned long baudrate, bool use_dte)
{
	u32 clk = imx_get_uartclk();

	if (!mxc_base)
		return;

	_mxc_serial_setbrg(mxc_base, clk, baudrate, use_dte);
}

char board_serial_getc(void)
{
	while (readl(&mxc_base->ts) & UTS_RXEMPTY)
		WATCHDOG_RESET();
	return (readl(&mxc_base->rxd) & URXD_RX_DATA); /* mask out status from upper word */
}

void board_serial_putc(const char c)
{
	if (!mxc_base)
		return;

	writel(c, &mxc_base->txd);

	/* wait for transmitter to be ready */
	while (!(readl(&mxc_base->ts) & UTS_TXEMPTY))
		WATCHDOG_RESET();
}

/*
 * Test whether a character is in the RX buffer
 */
static int one_time_rx_line_always_low_workaround_needed = 1;
int board_serial_tstc(void)
{
	if (!mxc_base)
		return 0;

	/* If receive fifo is empty, return false */
	if (readl(&mxc_base->ts) & UTS_RXEMPTY)
		return 0;

	/* Empty RX FIFO if receiver is stuck because of RXD line being low */
	if (one_time_rx_line_always_low_workaround_needed) {
		one_time_rx_line_always_low_workaround_needed = 0;
		if (!(readl(&mxc_base->sr2) & USR2_RDR)) {
			while (!(readl(&mxc_base->ts) & UTS_RXEMPTY)) {
				(void) readl(&mxc_base->rxd);
			}
			return 0;
		}
	}

	return 1;
}

int board_serial_init(void *base, unsigned long baudrate, bool use_dte)
{
	if (!base)
		return -1;

	mxc_base = (struct mxc_uart *) base;

	_mxc_serial_init(mxc_base, use_dte);

	mxc_serial_setbrg(baudrate, use_dte);

	return 0;
}

void board_serial_puts(const char *s)
{
	while (*s)
		board_serial_putc(*s++);
}
