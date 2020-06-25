
void setup_cli();

#ifdef CFG_CLI
enum cmd_device_enum;

struct cli_entry {
	char *name;
	void (*call)(enum cmd_device_enum, char *, int);
};

static const struct cli_entry cli_entries[];
#endif

