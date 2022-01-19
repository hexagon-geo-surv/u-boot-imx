// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021 Leica Geosystems AG
 */

#include <common.h>
#include <watchdog.h>
#include <console.h>
#include <leica_sep.h>

#define SEP_ACK_CMD           "SE203:X\r\n"
#define SEP_BOARD_ID          "SC111:\r\n"
#define SEP_PRODUCTION_FLAG   "SC135:\r\n"
/*Max length of a SEP cmd*/
#define SEP_CMD_LENGTH        16
/*Board id index in the payload array*/
#define BOARD_ID_IDX          6

/*
 * leica_sep_transfer
 *
 * -> Send SEP command 'SCXXX:\r\n'
 * -> Wait for reception of 'SRXXX,ABCD:<payload>\r\n'
 * -> Fills the input 'data' with the <payload>
 */
static int leica_sep_transfer(struct leica_sep_funcs *f, char *cmd,
			      char *data, size_t size, ulong timeout_ms)
{
	size_t bytes_received = 0;
	ulong start = get_timer(0);
	int cmd_length = 0;
	bool header_received = false, command_received = false;

	if(!f || !f->puts || !f->tstc || !f->getc || !cmd || !data)
		return -1;

	if(cmd[0] != 'S' || cmd[1] != 'C')
		return -1;

	while(cmd[cmd_length] != ':' && cmd[cmd_length] != '\0')
		cmd_length++;

	/* Empty receiver FIFO before sending our command*/
	while(f->tstc())
		f->getc();

	f->puts("\r\n");
	f->puts(cmd);
	cmd[1] = 'R';
	while(get_timer(0) < (start + timeout_ms))
	{
		if (!f->tstc())
			continue;

		if (size <= bytes_received)
			return -1;

		data[bytes_received] = f->getc();

		if (data[bytes_received] == '\n' &&
		    bytes_received >= 1 &&
		    data[bytes_received - 1] == '\r') {
			if (header_received) {
				bytes_received--;

				if (bytes_received >= 2 &&
				    data[bytes_received - 2] == '"')
					bytes_received--;

				command_received = true;
				break;
			}

			bytes_received = 0;
		}

		if (data[bytes_received] == ':') {
			/* Compare data with cmd. Data before ":" can be longer
			 * than cmd (including the CRC), this is why cmd_length
			 * is used for the comparison
			 */
			if (bytes_received >= cmd_length - 1 &&
			    !strncmp(data, cmd, cmd_length))
				header_received = true;

			bytes_received = 0;
			continue;
		}

		if (bytes_received == 0 && data[bytes_received] == '"')
			continue;

		bytes_received++;
	}

	if (!command_received)
		return -1;

	/* We substitute the last char with '\0' to terminate the payload
	 * string
	 */
	data[bytes_received] = '\0';

	return bytes_received;
}

/*
 * leica_get_sep
 *
 * Function to communicate with the MCU and get parsed data from the SEP
 * payload. The result is stored in the input 'data' string.
 * Available SEPs are: 'BOARD_ID', 'PRODUCTION_FLAG'.
 */
int leica_get_sep(struct leica_sep_funcs *f, enum leica_sep_type leica_sep,
		  char *data, int data_size)
{
	char command[SEP_CMD_LENGTH];
	int n_bytes;

	switch (leica_sep) {
		case(BOARD_ID):
			snprintf(command, sizeof(command), "%s", SEP_BOARD_ID);
			break;
		case(PRODUCTION_FLAG):
			snprintf(command, sizeof(command), "%s",
				 SEP_PRODUCTION_FLAG);
			break;
	}
	n_bytes = leica_sep_transfer(f, command, data, data_size, 500);
	if (n_bytes < 0) {
		printf("Error in SEP transfer\n");
		return -1;
	}
	if (leica_sep == BOARD_ID) {
		if (strlen(data) < BOARD_ID_IDX + 1) {
			printf("Error getting board id\n");
			return -1;
		}
		data[0] = data[BOARD_ID_IDX];
		data[1] = '\0';
	}
	return 0;
}

int leica_sep_send_ack(struct leica_sep_funcs *f, int boot_src)
{
	char data[] = SEP_ACK_CMD;

	if (!f || !f->puts)
		return -1;

	switch (boot_src) {
		case LEICA_SEP_ACK_BOOT_SRC_EMMC:
			data[6] = '2';
			break;
		case LEICA_SEP_ACK_BOOT_SRC_QSPI:
			data[6] = '1';
			break;
		default:
			return -1;
	}

	f->puts("\r\n");
	f->puts(data);

	return 0;
}
