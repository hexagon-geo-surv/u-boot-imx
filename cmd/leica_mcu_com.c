// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 Leica Geosystems AG
 */

#include <common.h>
#include <command.h>
#include <console.h>

__weak int board_mcu_set_cpu_ready(void)
{
	return -1;
}

static int do_leica_mcu_communications(cmd_tbl_t *cmdtp, int flag, int argc,
				       char * const argv[])
{
	const char *command;
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
	default:
		return CMD_RET_USAGE;
	}

	if (ret < 0)
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

#if CONFIG_IS_ENABLED(SYS_LONGHELP)
static char leica_mcu_comm_help_text[] =
	"leica_mcu_comm cpu_ready     Set CPU_READY\n"
	;
#endif

U_BOOT_CMD(leica_mcu_comm, 2, 0, do_leica_mcu_communications,
	   "Communicate with Leica MCUs",
#if CONFIG_IS_ENABLED(SYS_LONGHELP)
	   leica_mcu_comm_help_text
#endif
);
