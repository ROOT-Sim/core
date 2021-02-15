/**
 * @file core/arg_parse.h
 *
 * @brief Command line option parser
 *
 * A command line option parser mimicking a subset of GNU argp features.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#pragma once

/// A single parsable command line option
struct ap_option {
	/// The long option name
	const char *name;
	/// The key passed to the parsing function when encountering this option
	/** This value must be strictly lower than 2^14 */
	int key;
	/// The argument name for this option, shown in the \--usage text
	/** If not NULL, the argument is mandatory, else it is disallowed */
	const char *arg;
	/// The documentation of this option, shown in the \--help text
	/** This field can be set to NULL */
	const char *doc;
};

/// The special events keys passed to command line parsing functions
enum ap_event_key {
	/// Signals the start of the parsing process
	AP_KEY_INIT = 1 << 14,
	/// Signals the end of the parsing process
	AP_KEY_FINI
};

/// A set of options organized and parsed together
struct ap_section {
	/// The header printed before this section in the \--help text
	const char *header;
	/// The array of recognized options, terminated by a zeroed element
	struct ap_option *opts;
	/// The parsing function
	/** This gets called for events specified in the enum ap_event_key
	 *  or when an option for this section is encountered during parsing. */
	void (*parser)(int key, const char *arg);
};

/// A complete command line option parsing setup
struct ap_settings {
	/// A description of the program
	const char *prog_doc;
	/// A description of the program version
	const char *prog_version;
	/// The email address of the maintainer
	const char *prog_report;
	/// The array of supported sections, terminated by a zeroed element
	struct ap_section *sects;
};

extern void arg_parse_run(struct ap_settings *ap_s, char **argv);
extern const char *arg_parse_program_name(void);
extern void arg_parse_error(const char *fmt, ...);
