/**
 * @file core/arg_parse.c
 *
 * @brief Command line option parser
 *
 * A command line option parser mimicking a subset of GNU argp features.
 *
 * @copyright
 * Copyright (C) 2008-2020 HPDCS Group
 * https://rootsim.github.io/core
 *
 * This file is part of ROOT-Sim (ROme OpTimistic Simulator).
 *
 * ROOT-Sim is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; only version 3 of the License applies.
 *
 * ROOT-Sim is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ROOT-Sim; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <core/arg_parse.h>

#include <mm/mm.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/// Expected length of the terminal expressed in monospace characters width
#define SCREEN_LENGTH 80
/// Number of spaces to indent the \--help entries
#define HELP_INDENT 6
/// Minimum indentation of the documentation strings in the \--help entries
#define HELP_OPT_LEN_MIN 20
/// Maximum indentation of the documentation strings in the \--help entries
#define HELP_OPT_LEN_MAX 26
/// Minimum spaces between the option and its documentation in the \--help text
#define HELP_SPACES_MIN 3
/// Number of spaces to indent the \--usage text
#define USAGE_INDENT 11

/// The program name extracted from the command line
static const char *ap_pname;
/// The currently parsed struct ap_settings
static const struct ap_settings *ap_settings;

/// The keys used in the internal struct ap_option to handle base options
enum internal_opt_key {
	/// Identifies the \--help option
	AP_HELP = AP_KEY_FINI + 1,
	/// Identifies the \--usage option
	AP_USAGE,
	/// Identifies the \--version option
	AP_VERSION
};

/// The internal struct ap_option used to handle base options
static struct ap_option ap_internal_opts[] = {
	{"help", AP_HELP, NULL, "Give this help list"},
	{"usage", AP_USAGE, NULL, "Give a short usage message"},
	{"version", AP_VERSION, NULL, "Print program version"},
	{0}
};

/**
 * @brief Compares two ap_option structs (used to reorder options alphabetically)
 * @param a the first ap_option to compare
 * @param b the second ap_option to compare
 * @return a positive or negative value if a is respectively before or after b,
 *         0 otherwise (should never happen with correct options specifications)
 */
static int cmp_opts (const void *a, const void *b)
{
	const struct ap_option *oa = a, *ob = b;
	return strcmp(oa->name, ob->name);
}

/**
 * @brief Compares two ap_section structs (used to reorder sections alphabetically)
 * @param a the first ap_section to compare
 * @param b the second ap_section to compare
 * @return a positive or negative value if a must be considered respectively
 *         before or after b, 0 otherwise
 */
static int cmp_sects (const void *a, const void *b)
{
	const struct ap_section *oa = a, *ob = b;

	if (oa->header == NULL || ob->header == NULL) {
		return (ob->header == NULL) - (oa->header == NULL);
	}

	return strcmp(oa->header, ob->header);
}

/**
 * @brief Prints a string in the terminal indenting it and wrapping it
 * @param str the string to print
 * @param curr_i the number of characters already present in the current line
 *               of the terminal
 * @param indent the desired indentation level for the string express in number
 *               of spaces
 */
static void print_indented_string(const char *str, int curr_i, int indent)
{
	const char *p = str;

	while (*p) {
		while (*p && !isspace(*p)) ++p;

		int l = p - str;
		if (l + curr_i > SCREEN_LENGTH) {
			printf("\n%*c", indent, ' ');
			curr_i = indent;
		}
		printf("%.*s", l, str);
		curr_i += l;

		while(*p && isspace(*p)) {
			if (*p == '\n' || curr_i >= SCREEN_LENGTH) {
				printf("\n%*c", indent, ' ');
				curr_i = indent;
			}
			if (*p != '\n') {
				putchar(*p);
				++curr_i;
			}
			++p;
		}
		str = p;
	}
}

/**
 * @brief Prints the help text for a single option
 * @param name the name of the option
 * @param arg the documentation for the argument (if present, else NULL)
 * @param doc the documentation for the option (if present, else NULL)
 */
static void print_help_option(const char *name, const char *arg, const char *doc)
{
	int l = printf("%*c--%s%s%s", HELP_INDENT, ' ',
		       name, arg ? "=" : "", arg ? arg : "");

	if (l > HELP_OPT_LEN_MAX + HELP_INDENT) {
		l = HELP_OPT_LEN_MIN + HELP_SPACES_MIN + HELP_INDENT;
		printf("\n%*c", l, ' ');
	} else if (l < HELP_OPT_LEN_MIN + HELP_INDENT) {
		l += printf("%*c", HELP_OPT_LEN_MIN + HELP_SPACES_MIN +
			    HELP_INDENT - l, ' ');
	} else {
		l += printf("%*c", HELP_SPACES_MIN, ' ');
	}

	if(doc)
		print_indented_string(doc, l, HELP_OPT_LEN_MIN +
				      HELP_SPACES_MIN + HELP_INDENT);

	puts("");
}

/**
 * @brief Prints the help text for the whole argument parsing context
 */
static void print_help(void)
{
	printf("Usage: %s [OPTION...]\n\n%s\n", ap_pname, ap_settings->prog_doc);

	struct ap_section *s = ap_settings->sects;

	do {
		struct ap_option *o = s->opts;

		if (!o->name)
			continue;

		if (s->header)
			printf(" %s\n", s->header);

		do {
			print_help_option(o->name, o->arg, o->doc);
		} while ((o++)->name);

		puts("");
	} while ((s++)->opts != ap_internal_opts);

	printf("Report bugs to %s.\n", ap_settings->prog_report);
}

/**
 * @brief Prints the help text for a single option
 * @param name the name of the option
 * @param arg the documentation for the argument (if present, else NULL)
 * @param curr_i the number of characters already present in the current line
 *               of the terminal
 */
static int print_usage_option(const char *name, const char *arg, int curr_i)
{
	int l = snprintf(NULL, 0, " [--%s%s%s]", name,
			 arg ? "=" : "", arg ? arg : "");

	if (curr_i + l > SCREEN_LENGTH) {
		printf("\n%*c", USAGE_INDENT, ' ');
		curr_i = l + USAGE_INDENT;
	} else {
		curr_i += l;
	}

	printf(" [--%s%s%s]", name, arg ? "=" : "", arg ? arg : "");

	return curr_i;
}

/**
 * @brief Prints the usage text for the whole argument parsing context
 */
static void print_usage(void)
{
	int curr_i = printf("Usage: %s", ap_pname);

	struct ap_section *s = ap_settings->sects;

	do {
		struct ap_option *o = s->opts;

		while (o->name) {
			curr_i = print_usage_option(o->name, o->arg, curr_i);
			++o;
		}

	} while ((s++)->opts != ap_internal_opts);

	puts("");
}

/**
 * @brief The parsing function for the internal options
 *        (--help, --usage, --version)
 * @param key the currently parsed key
 * @param arg the currently parsed argument
 */
static void internal_opt_parse(int key, const char *arg)
{
	(void)arg;
	switch (key) {
		case AP_HELP:
			print_help();
			break;
		case AP_USAGE:
			print_usage();
			break;
		case AP_VERSION:
			puts(ap_settings->prog_version);
			break;
		default:
			return;
	}
	exit(0);
}

/**
 * @brief Sets up the ap_section structs and their ap_option structs by sorting
 *        them. The internal ap_section struct is also injected
 */
static void sort_and_setup_settings(void)
{
	struct ap_section *s = ap_settings->sects;
	size_t sects_cnt = 0;

	while (s[sects_cnt].opts) {
		struct ap_option *o = s[sects_cnt].opts;

		size_t opts_cnt = 0;

		while (o[opts_cnt].name) {
			++opts_cnt;
		}

		qsort(o, opts_cnt, sizeof(struct ap_option), cmp_opts);

		++sects_cnt;
	}

	qsort(s, sects_cnt, sizeof(struct ap_section), cmp_sects);

	// injects the internal options to ease the native options handling
	s[sects_cnt].header = NULL;
	s[sects_cnt].opts = ap_internal_opts;
	s[sects_cnt].parser = internal_opt_parse;
}

/**
 * @brief Undos the injection of the internal ap_section struct
 */
static void undo_setup_settings(void)
{
	struct ap_section *s = ap_settings->sects;
	while (s->opts != ap_internal_opts)
		++s;
	s->opts = NULL;
	s->parser = NULL;
}

/**
 * @brief Parses a single option by calling the appropriate parser or throwing
 *        an error
 * @param s the section of the option currently parsed
 * @param o the option currently parsed
 * @param arg the argument associated with the option (can be NULL)
 * @param arg_explicit a flag indicating if the argument has been supplied
 *                     explicitly (with an option formatted as --option=value)
 * @return 1 if a non explicit argument has been consumed during parsing,
 *         0 otherwise
 */
static int parse_option(struct ap_section *s, struct ap_option *o,
			const char *arg, bool arg_explicit)
{
	if (!arg && o->arg)
		arg_parse_error("option '--%s' requires an argument", o->name);

	if (arg && !o->arg) {
		if (arg_explicit)
			arg_parse_error(
				"option '--%s' does not require an argument", o->name);
		else if (arg[0] != '-')
			arg_parse_error("too many arguments");
		else
			arg = NULL;
	}

	s->parser(o->key, arg);

	return arg != NULL && !arg_explicit;
}

/**
 * @brief Parses a single option by calling the appropriate parser or throwing
 *        an error
 * @param o_name the option currently parsed
 * @param arg the argument associated with the option (can be NULL)
 * @return 1 if the argument has been consumed during parsing, 0 otherwise
 */
static int process_option(const char *o_name, const char *arg)
{
	unsigned max_s = 0;
	struct ap_option *cand_o = NULL;
	struct ap_section *cand_s = NULL;
	struct ap_section *s = ap_settings->sects;

	do {
		struct ap_option *o = s->opts;

		while (o->name) {
			unsigned i = 0;
			while(o_name[i] && o->name[i] == o_name[i])
				++i;

			if (o_name[i] == o->name[i])
				return parse_option(s, o, arg, false);

			if (max_s == i)
				cand_o = NULL;

			if (max_s < i) {
				max_s = i;
				cand_o = o;
				cand_s = s;
			}

			++o;
		}
	} while ((s++)->opts != ap_internal_opts);

	if (!max_s || (o_name[max_s] && o_name[max_s] != '='))
		arg_parse_error("unrecognized option '--%s'", o_name);

	if (!cand_o)
		arg_parse_error("ambiguous option '--%s'", o_name);

	if (o_name[max_s] == '=')
		return parse_option(cand_s, cand_o, &o_name[max_s + 1], true);

	return parse_option(cand_s, cand_o, arg, false);
}

/**
 * @brief Parses the command line options
 * @param ap_s the parsing settings (see struct ap_settings for more info)
 * @param argv the NULL terminated array of C strings from the command line
 */
void arg_parse_run(struct ap_settings *ap_s, char **argv)
{
	ap_pname = strrchr(*argv, '/');
	ap_pname = ap_pname ? ap_pname + 1 : *argv;

	ap_settings = ap_s;

	sort_and_setup_settings();

	struct ap_section *s = ap_s->sects;
	do {
		s->parser(AP_KEY_INIT, NULL);
	} while ((s++)->opts != ap_internal_opts);

	++argv;
	while (*argv) {
		const char *str = *argv;
		if (str[0] != '-')
			arg_parse_error("too many arguments");

		if (str[1] != '-')
			arg_parse_error("invalid option -- '%s'", &str[1]);

		argv += process_option(&str[2], *(argv + 1));
		++argv;
	}

	s = ap_s->sects;
	do {
		s->parser(AP_KEY_FINI, NULL);
	} while ((s++)->opts != ap_internal_opts);

	undo_setup_settings();
}

/**
 * @brief Gets the program name
 * @return a C string containing the program name, or NULL if arg_parse_run
 *           hasn't been called yet.
 */
const char *arg_parse_program_name(void)
{
	return ap_pname;
}

/**
 * @brief Prints a parsing related error message and exits with a bad exit code
 * @param fmt the printf-style format string
 * @param ... the arguments required to fill in the format string
 */
void arg_parse_error(const char *fmt, ...)
{
	fprintf(stderr, "%s: ", ap_pname);

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr,
		"\nTry `%s --help' or `%s --usage' for more information.\n",
		ap_pname, ap_pname);

	exit(64);
}
