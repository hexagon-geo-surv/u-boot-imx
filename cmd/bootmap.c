// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <console.h>

struct bootmap_switch {
	u32 address;
	u32 size;
} __packed;

static int do_get_active_bootmap(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	void *addr;
	struct bootmap_switch *bms;

	if (argc < 2)
		return CMD_RET_USAGE;

	addr = (void *)simple_strtoul(argv[1], NULL, 16);
	if (!addr) {
		printf("Bootmap-Switch address is NULL!\n");
		return CMD_RET_FAILURE;
	}

	bms = (struct bootmap_switch *) addr;

	bms->address = be32_to_cpu(bms->address);
	bms->size = be32_to_cpu(bms->size);

	if (bms->address % 512 || bms->size % 512) {
		printf("Bootmap not aligned!\n");
		return CMD_RET_FAILURE;
	}

	env_set_hex("bootmap.address", bms->address);
	env_set_hex("bootmap.blk", bms->address >> 9);
	env_set_hex("bootmap.size", bms->size);
	env_set_hex("bootmap.cnt", bms->size >> 9);

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	bootmap, 2, 0, do_get_active_bootmap,
	"Add address and size of Leica System 1600 bootmap to environment.",
	"bootmap <address>"
	"    <address>        Address of bootmap-switch (hex)\n"
	"Variables added to environment:\n"
	"    bootmap.address  Address of bootmap (hex)\n"
	"    bootmap.blk      Block of bootmap (hex)\n"
	"    bootmap.size     Size of bootmap (hex)\n"
	"    bootmap.cnt      Size of bootmap in blocks (hex)\n"
);
