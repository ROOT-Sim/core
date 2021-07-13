/**
 * @file test/lib/reflect.c
 *
 * @brief Test: reflection-like population of a struct from a json file
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <test.h>
#include <lib/config/jsmn.h>
#include <lib/config/reflect.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <float.h>


struct keyval {
	int id;
	char *type;
};

struct history {
	double *ppu;
};

struct conf {
	int id;
	char *type;
	char *name;
	double ppu;
	struct keyval *batters;
	struct keyval *topping;
	struct history *ppu_history;
};

struct autoconf_type_map struct_keyval[] = {{"id",   offsetof(struct keyval, id),   AUTOCONF_UNSIGNED, NULL, 0},
					    {"type", offsetof(struct keyval, type), AUTOCONF_STRING,   NULL, 0},
					    {0}};

struct autoconf_type_map struct_history[] = {{"ppu", offsetof(struct history, ppu),   AUTOCONF_ARRAY_DOUBLE, NULL, 0},
					    {0}};

struct autoconf_type_map struct_conf[] =
	{{"id",          offsetof(struct conf, id),          AUTOCONF_UNSIGNED,        NULL,             0},
	 {"type",        offsetof(struct conf, type),        AUTOCONF_STRING,          NULL,             0},
	 {"name",        offsetof(struct conf, name),        AUTOCONF_STRING,          NULL,             0},
	 {"ppu",         offsetof(struct conf, ppu),         AUTOCONF_DOUBLE,          NULL,             0},
	 {"available",   offsetof(struct conf, ppu),         AUTOCONF_BOOL,            NULL,             0},
	 {"batters",     offsetof(struct conf, batters),     AUTOCONF_ARRAY_OBJECT,    "struct keyval",  sizeof(struct keyval)},
	 {"topping",     offsetof(struct conf, topping),     AUTOCONF_ARRAY_OBJECT,    "struct keyval",  sizeof(struct keyval)},
	 {"ppu_history", offsetof(struct conf, ppu_history), AUTOCONF_OBJECT,          "struct history", sizeof(struct history)},
	 {0}};

struct autoconf_name_map autoconf_structs[] = {{"struct keyval",  struct_keyval},
					       {"struct conf",    struct_conf},
					       {"struct history", struct_history},
					       {0}};

char *conf_file = "{"
		  "	\"id\": 1,\n"
		  "	\"type\": \"donut\",\n"
		  "	\"name\": \"Cake\",\n"
		  "	\"ppu\": 0.55,\n"
		  "	\"available\": true,\n"
		  "	\"batters\":\n"
		  "		[\n"
		  "			{ \"id\": 1001, \"type\": \"Regular\" },\n"
		  "			{ \"id\": 1002, \"type\": \"Chocolate\" },\n"
		  "			{ \"id\": 1003, \"type\": \"Blueberry\" },\n"
		  "			{ \"id\": 1004, \"type\": \"Devil's Food\" }\n"
		  "		],\n"
		  "	\"topping\":\n"
		  "		[\n"
		  "			{ \"id\": 5001, \"type\": \"None\" },\n"
		  "			{ \"id\": 5002, \"type\": \"Glazed\" },\n"
		  "			{ \"id\": 5005, \"type\": \"Sugar\" },\n"
		  "			{ \"id\": 5007, \"type\": \"Powdered Sugar\" },\n"
		  "			{ \"id\": 5006, \"type\": \"Chocolate with Sprinkles\" },\n"
		  "			{ \"id\": 5003, \"type\": \"Chocolate\" },\n"
		  "			{ \"id\": 5004, \"type\": \"Maple\" }\n"
		  "		],\n"
		  "	\"ppu_history\": {\n"
		  "		\"ppu\": [0.23, 0.31, 0.37, 0.44, 0.49, 0.51]\n"
		  "	}\n"
		  "}\n";

// Expected tokenization sequence of the above
static unsigned int expected_tokens[] = {JSMN_OBJECT, JSMN_STRING, JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING,
					 JSMN_STRING, JSMN_STRING, JSMN_STRING, JSMN_PRIMITIVE, JSMN_STRING,
					 JSMN_PRIMITIVE, JSMN_STRING, JSMN_ARRAY, JSMN_OBJECT, JSMN_STRING,
					 JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_OBJECT, JSMN_STRING,
					 JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_OBJECT, JSMN_STRING,
					 JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_OBJECT, JSMN_STRING,
					 JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_STRING, JSMN_ARRAY, JSMN_OBJECT,
					 JSMN_STRING, JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_OBJECT,
					 JSMN_STRING, JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_OBJECT,
					 JSMN_STRING, JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_OBJECT,
					 JSMN_STRING, JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_OBJECT,
					 JSMN_STRING, JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_OBJECT,
					 JSMN_STRING, JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_OBJECT,
					 JSMN_STRING, JSMN_PRIMITIVE, JSMN_STRING, JSMN_STRING, JSMN_STRING,
					 JSMN_OBJECT, JSMN_STRING, JSMN_ARRAY, JSMN_PRIMITIVE, JSMN_PRIMITIVE,
					 JSMN_PRIMITIVE, JSMN_PRIMITIVE, JSMN_PRIMITIVE, JSMN_PRIMITIVE};

static void check_loaded_struct(struct conf *configuration)
{
	assert(configuration->id == 1);
	assert(strcmp(configuration->type, "donut") == 0);
	assert(strcmp(configuration->name, "Cake") == 0);
	assert(configuration->ppu - 0.55 < DBL_EPSILON);
	assert(configuration->batters != NULL);
	assert(configuration->batters[0].id == 1001);
	assert(configuration->batters[1].id == 1002);
	assert(configuration->batters[2].id == 1003);
	assert(configuration->batters[3].id == 1004);
	assert(strcmp(configuration->batters[0].type, "Regular") == 0);
	assert(strcmp(configuration->batters[1].type, "Chocolate") == 0);
	assert(strcmp(configuration->batters[2].type, "Blueberry") == 0);
	assert(strcmp(configuration->batters[3].type, "Devil's Food") == 0);
	assert(configuration->topping != NULL);
	assert(configuration->topping[0].id == 5001);
	assert(configuration->topping[1].id == 5002);
	assert(configuration->topping[2].id == 5005);
	assert(configuration->topping[3].id == 5007);
	assert(configuration->topping[4].id == 5006);
	assert(configuration->topping[5].id == 5003);
	assert(configuration->topping[6].id == 5004);
	assert(strcmp(configuration->topping[0].type, "None") == 0);
	assert(strcmp(configuration->topping[1].type, "Glazed") == 0);
	assert(strcmp(configuration->topping[2].type, "Sugar") == 0);
	assert(strcmp(configuration->topping[3].type, "Powdered Sugar") == 0);
	assert(strcmp(configuration->topping[4].type, "Chocolate with Sprinkles") == 0);
	assert(strcmp(configuration->topping[5].type, "Chocolate") == 0);
	assert(strcmp(configuration->topping[6].type, "Maple") == 0);
	assert(configuration->ppu_history != NULL);
	assert(configuration->ppu_history->ppu != NULL);
	assert(configuration->ppu_history->ppu[0] - 0.23 < DBL_EPSILON);
	assert(configuration->ppu_history->ppu[1] - 0.31 < DBL_EPSILON);
	assert(configuration->ppu_history->ppu[2] - 0.37 < DBL_EPSILON);
	assert(configuration->ppu_history->ppu[3] - 0.44 < DBL_EPSILON);
	assert(configuration->ppu_history->ppu[4] - 0.49 < DBL_EPSILON);
	assert(configuration->ppu_history->ppu[5] - 0.51 < DBL_EPSILON);

}

static void autoconf_test(void)
{
	struct conf configuration;
	memset(&configuration, 0, sizeof(configuration));
	assert(load_json_from_string(&configuration, conf_file) == 0);
	check_loaded_struct(&configuration);
}

static void autoconf_test_from_file(void)
{
	struct conf configuration;
	FILE *tmp = tmpfile();
	memset(&configuration, 0, sizeof(configuration));

	fputs(conf_file, tmp);

	assert(load_json_from_file(&configuration, tmp) == 0);
	check_loaded_struct(&configuration);
}

static void jsmn_test(void)
{
	jsmn_parser parser;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */

	jsmn_init(&parser);
	int r = jsmn_parse(&parser, conf_file, strlen(conf_file), t, sizeof(t) / sizeof(t[0]));
	assert(r >= 0);

	// Check the expected number of tokens
	assert(r == sizeof(expected_tokens) / sizeof(expected_tokens[0]));

	// Check the expected tokenization sequence
	for(int i = 0; i < r; i++)
		assert(t[i].type == expected_tokens[i]);
}

static int reflect_test(void)
{
	jsmn_test();
	autoconf_test();
	autoconf_test_from_file();
	return 0;
}

const struct test_config test_config = {.threads_count = 1, .test_fnc = reflect_test};
