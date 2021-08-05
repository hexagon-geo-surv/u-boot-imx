#ifndef _LEICA_SEP_H_
#define _LEICA_SEP_H_

struct leica_sep_funcs {
	void (*puts)(const char *s);
	int (*tstc)(void);
	char (*getc)(void);
};

struct leica_sep_board_id {
	char material_number[6];
	char board_index;
	char production_date[6];
	char consecutive_number[4];
} __packed;

enum {
	LEICA_SEP_ACK_BOOT_SRC_EMMC,
	LEICA_SEP_ACK_BOOT_SRC_QSPI,
	LEICA_SEP_ACK_BOOT_SRC_UNKNOWN,
};

int leica_sep_send_ack(struct leica_sep_funcs *f, int boot_src);
int leica_sep_get_board_id(struct leica_sep_funcs *f, struct leica_sep_board_id *version);


#endif /* _LEICA_SEP_H_ */
