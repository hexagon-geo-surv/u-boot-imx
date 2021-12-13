/* SPDX-License-Identifier: GPL-2.0+
 *
 * Copyright (C) 2021 Leica Geosystems AG
 */

#ifndef _LEICA_SEP_H_
#define _LEICA_SEP_H_

struct leica_sep_funcs {
	void (*puts)(const char *s);
	int (*tstc)(void);
	char (*getc)(void);
};

enum {
	LEICA_SEP_ACK_BOOT_SRC_EMMC,
	LEICA_SEP_ACK_BOOT_SRC_QSPI,
	LEICA_SEP_ACK_BOOT_SRC_UNKNOWN,
};

enum leica_sep_type {
	BOARD_ID,
	PRODUCTION_FLAG,
};

int leica_sep_send_ack(struct leica_sep_funcs *f, int boot_src);
int leica_get_sep(struct leica_sep_funcs *f, enum leica_sep_type leica_sep,
		  char *data, int data_size);

#endif /* _LEICA_SEP_H_ */
