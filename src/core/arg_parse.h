#pragma once

struct ap_section {
	const char *header;
	struct ap_option *opts;
	int (*parser)(int key, const char *arg);
};

struct ap_settings
{
	const char *prog_doc;
	const char *prog_version;
	const char *prog_report;
	struct ap_section *sects;
};

extern void arg_parse_run(struct ap_settings *ap_s, char **argv);
extern void arg_parse_error(const char *fmt, ...);
