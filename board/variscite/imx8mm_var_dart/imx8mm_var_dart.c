/*
 * Copyright 2018 NXP
 * Copyright 2018-2020 Variscite Ltd.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <asm/arch/clock.h>
#include <usb.h>
#include <dm.h>
#include <leica_sep.h>

#include "../common/imx8_eeprom.h"
#include "imx8mm_var_dart.h"
#include "board_serial.h"

DECLARE_GLOBAL_DATA_PTR;

extern int var_setup_mac(struct var_eeprom *eeprom);
static struct leica_sep_funcs sep_funcs = {0};

#ifdef CONFIG_SPL_BUILD
#define GPIO_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1 | PAD_CTL_PUE | PAD_CTL_PE)
#define ID_GPIO 	IMX_GPIO_NR(2, 11)

static iomux_v3_cfg_t const id_pads[] = {
	IMX8MM_PAD_SD1_STROBE_GPIO2_IO11 | MUX_PAD_CTRL(GPIO_PAD_CTRL),
};

int get_board_id(void)
{
	static int board_id = UNKNOWN_BOARD;

	if (board_id != UNKNOWN_BOARD)
		return board_id;

	imx_iomux_v3_setup_multiple_pads(id_pads, ARRAY_SIZE(id_pads));
	gpio_request(ID_GPIO, "board_id");
	gpio_direction_input(ID_GPIO);

	board_id = gpio_get_value(ID_GPIO) ? DART_MX8M_MINI : VAR_SOM_MX8M_MINI;

	return board_id;
}
#else
int get_board_id(void)
{
	static int board_id = UNKNOWN_BOARD;

	if (board_id != UNKNOWN_BOARD)
		return board_id;

	if (of_machine_is_compatible("variscite,imx8mm-var-som"))
		board_id = VAR_SOM_MX8M_MINI;
	else if (of_machine_is_compatible("variscite,imx8mm-var-dart"))
		board_id = DART_MX8M_MINI;
	else
		board_id = UNKNOWN_BOARD;

	return board_id;
}
#endif

int var_get_som_rev(struct var_eeprom *ep)
{
	if (ep->somrev == 0)
		return SOM_REV_10;
	else
		return SOM_REV_11;
}

static int leica_get_boot_src()
{
	struct src *psrc = (struct src *)SRC_BASE_ADDR;
	unsigned reg = readl(&psrc->sbmr1) >> 12;
	int ret = -1;

	switch (reg & 0x7) {
	case 0x2:
		ret = LEICA_SEP_ACK_BOOT_SRC_EMMC;
		puts("Booted from eMMC\n");
		break;
	case 0x4:
		ret = LEICA_SEP_ACK_BOOT_SRC_QSPI;
		puts("Booted from QSPI\n");
		break;
	default:
		ret = LEICA_SEP_ACK_BOOT_SRC_UNKNOWN;
		puts("Unknown Boot source\n");
		break;
	}

	return ret;
}

#define UART_PAD_CTRL		(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define CPU_RDY_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1 | PAD_CTL_PUE)
#define WDOG_PAD_CTRL		(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)
#define CPU_RDY_GPIO 		IMX_GPIO_NR(1, 14)

static iomux_v3_cfg_t const uart1_pads[] = {
	IMX8MM_PAD_UART1_RXD_UART1_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART1_TXD_UART1_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const uart2_pads[] = {
       IMX8MM_PAD_UART2_RXD_UART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
       IMX8MM_PAD_UART2_TXD_UART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const uart4_pads[] = {
	IMX8MM_PAD_UART4_RXD_UART4_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
	IMX8MM_PAD_UART4_TXD_UART4_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
	IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

static iomux_v3_cfg_t const cpu_rdy_pads[] = {
	IMX8MM_PAD_GPIO1_IO14_GPIO1_IO14  | MUX_PAD_CTRL(CPU_RDY_PAD_CTRL),
};

extern struct mxc_uart *mxc_base;

int board_early_init_f(void)
{
	int id;
	struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

	imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

	set_wdog_reset(wdog);

	imx_iomux_v3_setup_multiple_pads(cpu_rdy_pads, ARRAY_SIZE(cpu_rdy_pads));

	id = get_board_id();

	if (id == DART_MX8M_MINI) {
		init_uart_clk(0);
		imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
	}else if (id == VAR_SOM_MX8M_MINI) {
		init_uart_clk(3);
		mxc_base = (struct mxc_uart *)UART4_BASE_ADDR;
		imx_iomux_v3_setup_multiple_pads(uart4_pads, ARRAY_SIZE(uart4_pads));
	}

	return 0;
}

#ifdef CONFIG_FEC_MXC
static int setup_fec(void)
{
	struct iomuxc_gpr_base_regs *gpr =
		(struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

	/* Use 125M anatop REF_CLK1 for ENET1, not from external */
	clrsetbits_le32(&gpr->gpr[1], 0x2000, 0);

	return 0;
}
#endif

#ifdef CONFIG_CI_UDC
#define USB_OTG1_ID_GPIO  IMX_GPIO_NR(1, 10)

static iomux_v3_cfg_t const usb_pads[] = {
	IMX8MM_PAD_GPIO1_IO10_GPIO1_IO10  | MUX_PAD_CTRL(NO_PAD_CTRL),
};

int board_usb_init(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, true);

	return 0;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	imx8m_usb_power(index, false);

	return 0;
}

/* Used only on VAR-SOM-MX8M-MINI Rev1.0 (with extcon) */
int board_ehci_usb_phy_mode(struct udevice *dev)
{
	if (dev->seq == 1)
		return USB_INIT_HOST;
	else
		return gpio_get_value(USB_OTG1_ID_GPIO) ?
			USB_INIT_DEVICE : USB_INIT_HOST;
}

static void setup_usb(void)
{
	struct var_eeprom *ep = VAR_EEPROM_DATA;
	int som_rev = var_get_som_rev(ep);

	if ((get_board_id() == VAR_SOM_MX8M_MINI) &&
	    (som_rev == SOM_REV_10)) {
		imx_iomux_v3_setup_multiple_pads(usb_pads, ARRAY_SIZE(usb_pads));
		gpio_request(USB_OTG1_ID_GPIO, "usb_otg1_id");
		gpio_direction_input(USB_OTG1_ID_GPIO);
	}
}
#endif

int board_mcu_set_cpu_ready(void)
{
	int ret;

	ret = gpio_request(CPU_RDY_GPIO, "cpu_rdy");
	if (ret < 0)
		return ret;

	return gpio_direction_output(CPU_RDY_GPIO, 1);
}

int get_leica_board_id(char *data, int data_size)
{
	return leica_get_sep(&sep_funcs, BOARD_ID, data, data_size);
}

int get_production_flag(char *data, int data_size)
{
	return leica_get_sep(&sep_funcs, PRODUCTION_FLAG, data, data_size);
}

int board_init(void)
{
#ifdef CONFIG_FEC_MXC
	setup_fec();
#endif

#ifdef CONFIG_CI_UDC
	setup_usb();
#endif

	return 0;
}

static int init_mcu_uart(struct leica_sep_funcs *sep_funcs)
{
	init_uart_clk(1);

	imx_iomux_v3_setup_multiple_pads(uart2_pads, ARRAY_SIZE(uart2_pads));

	if (board_serial_init((void*)UART2_BASE_ADDR, 115200, false) < 0)
		return -1;

	sep_funcs->puts = &board_serial_puts;
	sep_funcs->getc = &board_serial_getc;
	sep_funcs->tstc = &board_serial_tstc;

	return 0;
}

#define SDRAM_SIZE_STR_LEN 5
int board_late_init(void)
{
	int som_rev;
	char sdram_size_str[SDRAM_SIZE_STR_LEN];
	int id = get_board_id();
	int boot_src = leica_get_boot_src();

	struct var_eeprom *ep = VAR_EEPROM_DATA;

	if (init_mcu_uart(&sep_funcs) == 0) {
		if (leica_sep_send_ack(&sep_funcs, boot_src) < 0)
			puts("Failed to send SEP ACK!\n");
	}
	else {
		printf("Failed to initilize MCU UART\n");
	}


#ifdef CONFIG_FEC_MXC
	var_setup_mac(ep);
#endif
	var_eeprom_print_prod_info(ep);

	som_rev = var_get_som_rev(ep);

	snprintf(sdram_size_str, SDRAM_SIZE_STR_LEN, "%d", (int) (gd->ram_size / 1024 / 1024));
	env_set("sdram_size", sdram_size_str);

	if (id == VAR_SOM_MX8M_MINI) {
		env_set("board_name", "VAR-SOM-MX8M-MINI");
		env_set("console", "ttymxc3,115200");
		if (som_rev == SOM_REV_10)
			env_set("som_rev", "som_rev10");
		else
			env_set("som_rev", "som_rev11");
	}
	else if (id == DART_MX8M_MINI)
		env_set("board_name", "DART-MX8M-MINI");

#ifdef CONFIG_ENV_IS_IN_MMC
	board_late_mmc_env_init();
#endif

	return 0;
}
