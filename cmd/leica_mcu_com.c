// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 Leica Geosystems AG
 */

#include <common.h>
#include <command.h>
#include <console.h>

#define RESPONSE_LENGTH  128

__weak int board_mcu_set_cpu_ready(void)
{
	return -1;
}

__weak int get_leica_board_id(char *data, int data_size)
{
	return -1;
}

__weak int get_production_flag(char *data, int data_size)
{
	return -1;
}

static int do_leica_mcu_communications(cmd_tbl_t *cmdtp, int flag, int argc,
				       char * const argv[])
{
	const char *command;
	char data[RESPONSE_LENGTH];
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;

	command = argv[1];

	switch (*command) {
	case 'c':
		ret = board_mcu_set_cpu_ready();
		if (ret < 0)
			printf("Set CPU_READY failed");
		break;
	case 'b':
		ret = get_leica_board_id(data, sizeof(data));
		if (ret < 0) {
			printf("Get board id failed\n");
			break;
		}
		printf("::::: BOARD ID: %s\n", data);
		if (argc > 2)
			env_set(argv[2], data);
		break;
	case 'p':
		ret = get_production_flag(data, sizeof(data));
		if (ret < 0) {
			printf("Get production flag failed\n");
			break;
		}
		printf("::::: PRODUCTION FLAG: %s\n", data);
		if (argc > 2)
			env_set(argv[2], data);
		break;
	default:
		return CMD_RET_USAGE;
	}

	if (ret < 0)
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

#if CONFIG_IS_ENABLED(SYS_LONGHELP)
static char leica_mcu_comm_help_text[] =
	"leica_mcu_comm cpu_ready                  Set CPU_READY\n"
	"leica_mcu_comm board_id                   Print the received board_id\n"
	"leica_mcu_comm board_id <env_var>         Print board_id and set the <env_var> environment variable\n"
	"leica_mcu_comm production_flag            Print the received production_flag\n"
	"leica_mcu_comm production_flag <env_var>  Print the received production_flag and set the <env_var> environment variable\n"
	;
#endif

U_BOOT_CMD(leica_mcu_comm, 3, 0, do_leica_mcu_communications,
	   "Communicate with Leica MCUs",
#if CONFIG_IS_ENABLED(SYS_LONGHELP)
	   leica_mcu_comm_help_text
#endif
);
