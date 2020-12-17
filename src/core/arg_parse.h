#pragma once

struct ap_section {
	const char *header; //!< the header printed before this section options
	struct ap_option *opts; //!< the array of options, terminated by a zeroed element
	void (*parser)(int key, const char *arg); //!< the parsing function
};

struct ap_settings
{
	const char *prog_doc; //!< a C string describing the program
	const char *prog_version; //!< a C string describing the program version
	const char *prog_report; //!< a C string containing the address of the maintainer
	struct ap_section *sects; //!< the array of sections, terminated by a zeroed element
};

extern void arg_parse_run(struct ap_settings *ap_s, char **argv);
extern const char *arg_parse_program_name(void);
extern void arg_parse_error(const char *fmt, ...);
