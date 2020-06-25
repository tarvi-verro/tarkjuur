#define CFG_CLI
#include "cli.h"
#include "cmd.h"
#include "conf.h"
#include "assert-c.h"
#include "rcc-c.h"
#include <string.h>
#include <ctype.h>

#define CLI_ENTRIES_LENGTH (sizeof(cli_entries)/sizeof(struct cli_entry))

static struct {
	int name_length;
} cli_entry_infos[CLI_ENTRIES_LENGTH];

static struct cmd_device_info {
	int displayed;
} cmd_device_infos[CMD_DEVICE_MAX] = { 0 };

void setup_cli()
{
	for (int i = 0; i < CLI_ENTRIES_LENGTH; i++)
		cli_entry_infos[i].name_length = strlen(cli_entries[i].name);
}

void cli_help(enum cmd_device_enum d, char *ln, int l)
{
	// TODO: Fix USB cmd_reply instead of the weird sleeps.
	cmd_reply(d, "\r\nAvailable commands: ");
	sleep_busy(1000*1000); // wait 2ms

	for (int i = 0; i < CLI_ENTRIES_LENGTH; i++) {
		if (i) cmd_reply(d, ", ");
		sleep_busy(1000*1000); // wait 2ms
		cmd_reply(d, cli_entries[i].name);
		sleep_busy(1000*1000); // wait 2ms
	}
	sleep_busy(1000*1000); // wait 2ms

	cmd_reply(d, ".\r\n");
}

static void cli_process(enum cmd_device_enum d, char *c, int l)
{
	assert(cli_entry_infos[0].name_length != 0);

	for (int i = 0; i < CLI_ENTRIES_LENGTH; i++) {
		int entry_length = cli_entry_infos[i].name_length;
		if (l < entry_length || memcmp(c, cli_entries[i].name, entry_length))
			continue;
		cli_entries[i].call(d, c + entry_length, l - entry_length);
		return;
	}
	cmd_reply(d, "\r\nUnknown command.\r\n");
}

void cli_handler_cb(enum cmd_device_enum d, struct cmd_line *n)
{
	struct cmd_device_info *devinf = cmd_device_infos + d;

	if (devinf->displayed == n->length)
		return;

	if (n->length > sizeof(n->line) - 1) {
		cmd_reply(d, "\r\nLine buffer overflow.\r\n");
		devinf->displayed = n->length = 0;
		return;
	}

	for (int i = devinf->displayed; i < n->length; i++) {
		char c = n->line[i];

		if (c == '\177') {
			n->length = devinf->displayed = devinf->displayed - 1;
			if (n->length < 0)
				n->length = devinf->displayed = 0;
			else
				cmd_reply(d, "\b \b");
			return;
		}
		if (c != '\r' && !isprint(c)) {
			n->length = devinf->displayed;
			return;
		}

		if (c != '\r')
			continue;

		cli_process(d, n->line, i);
		devinf->displayed = n->length = 0;
		return;
	}

	if (devinf->displayed != n->length) {
		n->line[n->length] = '\0';
		cmd_reply(d, n->line + devinf->displayed);
	}
	devinf->displayed = n->length;
}

