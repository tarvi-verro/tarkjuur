#pragma once

enum cmd_device_enum;
struct cmd_line {
	int length;
	char line[64];
};
typedef void (*cmd_handler_cb)(enum cmd_device_enum, struct cmd_line *);

/**
 * cmd_handler_set: set another handler for the command device
 *
 * returns the previous handler, reset to that when done
 */
cmd_handler_cb cmd_handler_set(enum cmd_device_enum, cmd_handler_cb);

void cmd_reply(enum cmd_device_enum, char *);
extern void cmd_update();
// Returns the number of characters copied
int cmd_received_cpy(enum cmd_device_enum, char *, int);

#ifdef CFG_CMD
struct cmd_configuration {
	cmd_handler_cb cb;
	void (* const reply)(char *);
	struct cmd_line cmd;
};

static struct cmd_configuration cfg_cmd[];
#endif
