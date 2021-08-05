// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <watchdog.h>
#include <console.h>
#include <leica_sep.h>

#define SEP_ACK_CMD   		"SE104:X\r\n"
#define SEP_BOARD_ID  		"SC110:\r\n"

/*
 * leica_sep_receive
 *
 * -> Send SEP command 'SCXXX:\r\n'
 * -> Wait for reception of 'SCXXX,ABCD:<payload>\r\n'
 */
static int leica_sep_receive(struct leica_sep_funcs *f, char *cmd, char *data, size_t size, ulong timeout_ms)
{
	size_t bytes_received = 0;
	ulong start = get_timer(0);
	int cmd_length = 0;
	int command_received = 0;
	char debug[1024] = {0};
	char *d = debug;

	if(!f || !f->puts || !f->tstc || !f->getc || !cmd || !data)
		return -1;

	while(cmd[cmd_length] != ':' && cmd[cmd_length] != '\0')
		cmd_length++;

	if (size < cmd_length)
		return -1;

	f->puts(cmd);
	*d++ = '-';
	while(get_timer(0) < (start + timeout_ms))
	{
		if (!f->tstc())
			continue;

		data[bytes_received++] = f->getc();
		*d++ = data[bytes_received -1];

		if ((data[bytes_received - 1] == '\n') || (bytes_received >= size)) {
			if (bytes_received >= cmd_length &&
			    !strncmp(data, cmd, cmd_length))
			{
				command_received = 1;
				break;
			}
			*d++ = '-';
			bytes_received = 0;
		}
	}
	debug[sizeof(debug) - 1] = 0;
	printf("[SEP DBG]:\n%s\n", debug);

	if (!command_received)
		return -1;

	return bytes_received;
}

int leica_sep_get_board_id(struct leica_sep_funcs *f, struct leica_sep_board_id *board_id)
{
	char data[512];
	int n_bytes;
	size_t n_header = sizeof(SEP_BOARD_ID) - 1 + sizeof(",XXXX") - 1;

	if(!board_id)
		return -1;

	n_bytes = leica_sep_receive(f, SEP_BOARD_ID, data, sizeof(data), 500);
	if (n_bytes < (int)(n_header + sizeof(*board_id))) {
		printf("[SEP DBG]: Receive failed. Using test-data... Remove this!!! \n");
		memcpy(data, "SC110,abcd:916761A2107310001\r\n", sizeof("SC110,abcd:916761A2107310001\r\n") - 1);
		//return -1;
	}

	memcpy(board_id, &data[n_header - (sizeof("\r\n") - 1)], sizeof(*board_id));

	return 0;
}

int leica_sep_send_ack(struct leica_sep_funcs *f, int boot_src)
{
	char data[] = SEP_ACK_CMD;

	if(!f || !f->puts)
		return -1;

	switch(boot_src) {
		case LEICA_SEP_ACK_BOOT_SRC_EMMC:
			data[6] = '2';
			break;
		case LEICA_SEP_ACK_BOOT_SRC_QSPI:
			data[6] = '1';
			break;
		default:
			return -1;
	}

	f->puts(data);

	return 0;
}
