#define CFG_CMD
#include "cmd.h"
#include "conf.h"
#include "assert-c.h"
#include <string.h>
#include <stdlib.h>


cmd_handler_cb cmd_handler_set(enum cmd_device_enum d, cmd_handler_cb cb)
{
	assert(d >= 0 && d < CMD_DEVICE_MAX);
	cmd_handler_cb rval = cfg_cmd[d].cb;
	cfg_cmd[d].cb = cb;
	return rval;
}

void cmd_reply(enum cmd_device_enum d, char *n)
{
	assert(d >= 0 && d < CMD_DEVICE_MAX);
	cfg_cmd[d].reply(n);
}

int cmd_received_cpy(enum cmd_device_enum d, char *dat, int l)
{
	struct cmd_line *n = &cfg_cmd[d].cmd;

	int w_size = sizeof(n->line) - n->length;
	char *w = n->line + n->length;

	int cpy_length = l <= w_size ? l : w_size;
	memcpy(w, dat, cpy_length);

	n->length += l;

	return cpy_length;
}

void cmd_update()
{
	for (enum cmd_device_enum d = 0; d < CMD_DEVICE_MAX; d++) {
		struct cmd_configuration *dev = cfg_cmd + d;
		dev->cb(d, &dev->cmd);
	}
}

