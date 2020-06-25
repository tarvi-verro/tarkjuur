#pragma once

#ifdef CFG_USB
static const struct usb_configuration_ep cfg_usb_ep[] = {
	{
		.rx_size = 64,
		.tx_size = 64,
		.epnr_init = {
			.ea = 0,
			.ep_type = USB_EP_TYPE_CONTROL,
			.stat_tx = USB_STAT_NAK,
			.stat_rx = USB_STAT_VALID,
			.ep_kind = 0,
		},
	},
	{
		.rx_size = 32,
		.epnr_init = {
			.ea = 1,
			.ep_type = USB_EP_TYPE_BULK,
			.stat_rx = USB_STAT_VALID,
			.stat_tx = USB_STAT_DISABLED,
		},
	},
	{
		.tx_size = 64,
		.epnr_init = {
			.ea = 2,
			.ep_type = USB_EP_TYPE_BULK,
			.stat_rx = USB_STAT_DISABLED,
			.stat_tx = USB_STAT_DISABLED,
		},
	},
};
#endif

#ifdef CFG_ASSERT
static const struct assert_configuration cfg_assert = {
	.led = PC13,
};
#endif

enum cmd_device_enum {
	CMD_DEVICE_USB,
	CMD_DEVICE_MAX,
};
#ifdef CFG_CMD
void cli_handler_cb(enum cmd_device_enum, struct cmd_line *); // cli.c
extern void usb_reply(char *); // usb-pck.c

static struct cmd_configuration cfg_cmd[] = {
	[CMD_DEVICE_USB] {
		.cb = cli_handler_cb,
		.reply = usb_reply,
		.cmd = {
			.length = 0,
			.line = {0},
		},
	},
};
#endif

#ifdef CFG_CLI
enum cmd_device_enum;
extern void measure_cli_call(enum cmd_device_enum d, char *, int l);
extern void cli_help(enum cmd_device_enum d, char *, int l);

static const struct cli_entry cli_entries[] = {
	{ "measure", measure_cli_call },
	{ "help", cli_help },
	{ "?", cli_help }
};
#endif

