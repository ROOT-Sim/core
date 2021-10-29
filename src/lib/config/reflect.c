/**
 * @file lib/config/reflect.c
 *
 * @brief Load a json file and populate a set of structs
 *
 * A set of structs annotated as configuration structs can be automatically filled by
 * this module. This significantly simplifies the configuration and initialization of
 * a simulation model.
 *
 * SPDX-FileCopyrightText: 2008-2021 HPDCS Group <rootsim@googlegroups.com>
 * SPDX-License-Identifier: GPL-3.0-only
 */
#include <lib/config/reflect.h>
#include <datatypes/vector.h>
#include <mm/mm.h>

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

//! This is populated by the rootsim compiler
extern struct autoconf_name_map autoconf_structs[];

//! This struct is given for the use in conjunction with next_get_token function().
//! Treat it as an opaque data type.
struct gnt_closure_t {
	int a;
	unsigned b;
};

//! use example: struct gnt_closure_t iterator = GNT_CLOSURE_INITIALIZER;
#define GNT_CLOSURE_INITIALIZER {0, 0}


//! This typedef is useful in avoiding accidental token modifications
typedef const jsmntok_t c_jsmntok_t;


static int load_json_object(void *struct_ptr, const char *struct_name, c_jsmntok_t *tokens, c_jsmntok_t *obj,
	const char *json_string);

static int strcmp_token(const char *json_string, c_jsmntok_t *t, const char *str);


/**
 * This is a useful function which iterates over the children tokens
 * of the JSON object or array represented by token obj.
 * Keep in mind that in case obj is an object, this function will iterate over keys
 * (corresponding values can be obtained by taking the key+1 token)
 * @param tokens the root token of the JSMN parse
 * @param obj the JSON array or object token we want to iterate over
 * @param closure the struct needed to keep state of the iteration, the definition and the initializer are given above
 * @return the next value token in case of an array, the next key token in case of an object, NULL at iteration end
 */
static c_jsmntok_t *get_next_token(c_jsmntok_t *tokens, c_jsmntok_t *obj, struct gnt_closure_t *closure)
{
	// we expect obj to be an object or an array
	assert(obj->type == JSMN_OBJECT || obj->type == JSMN_ARRAY);
	if(obj->size <= closure->a) {
		return NULL;
	}

	closure->a++;
	closure->b++;

	while((obj + closure->b)->parent != (obj - tokens)) {
		closure->b++;
	}

	return obj + closure->b;
}

/**
 * This function directly retrieves from the object token obj the value token associated with the supplied key.
 * @param tokens the root token of the JSMN parse
 * @param json_string the start of the json_string string (the JSON document)
 * @param obj the JSON object token
 * @param key the key
 * @return the value token associated in t_obj associated with the given key, NULL in case of failure
 */
static c_jsmntok_t *
get_value_token_by_key(c_jsmntok_t *tokens, const char *json_string, c_jsmntok_t *obj, const char *key)
{
	c_jsmntok_t *t_aux;

	// we expect obj to be an object
	assert(obj->type == JSMN_OBJECT);

	struct gnt_closure_t closure = GNT_CLOSURE_INITIALIZER;

	while((t_aux = get_next_token(tokens, obj, &closure)) != NULL) {
		/*
		 *  this stuff is not greatly documented in jsmn, anyway it turns out that
		 *  strings acting as object keys have child count set to 1.
		 *  This is just another unnecessary check.
		 */
		if(t_aux->type == JSMN_STRING && t_aux->size == 1) {
			if(strcmp_token(json_string, t_aux, key) == 0) {
				// this is token representing the value associated with the key we found
				return t_aux + 1;
			}
		}
	}
	return NULL;
}

/**
 * This function parses a floating point double value from a JSMN token
 * @param json_string the start of the json_string string (the JSON document)
 * @param t the JSON number token to parse
 * @param result a pointer to a valid double variable which will hold the parsed value
 *
 * @return 0 in case of success, -1 for failure
 */
static int parse_double_token(const char *json_string, c_jsmntok_t *t, double *result)
{
	char buff[64];
	size_t size;
	double tmp;
	char *check;

	assert(t->type == JSMN_PRIMITIVE);

	size = (size_t) (t->end - t->start);
	size = size < (64 - 1) ? size : (64 - 1);

	memcpy(buff, &json_string[t->start], size);
	buff[size] = '\0';

	tmp = strtod(buff, &check);
	if(buff == check) {
		return -1;
	}

	*result = tmp;
	return 0;
}

/**
 * This function parses a unsigned int value from a JSMN token
 * @param json_string the start of the json_string string (the JSON document)
 * @param t the JSON number token to parse
 * @param result a pointer to a valid unsigned int variable which will hold the parsed value
 * @return 0 in case of success, -1 for failure
 */
static int parse_unsigned_token(const char *json_string, c_jsmntok_t *t, unsigned *result)
{
	double tmp;

	if(parse_double_token(json_string, t, &tmp) < 0
	   || tmp < 0.0
	   || tmp > UINT_MAX
	   || tmp > (unsigned) tmp
	   || tmp < (unsigned) tmp) {
		return -1;
	}

	*result = (unsigned) tmp;
	return 0;
}

/**
 * This function behaves like standard strcmp() with the difference that the first argument is a JSMN token
 * @param json_string the start of the json_string string (the JSON document)
 * @param t the JSON string token to compare
 * @param str the string to compare against the string token
 * @return a positive, zero or negative value if the string token is respectively, more than, equal
 * or less than the string @p str.
 */
static int strcmp_token(const char *json_string, c_jsmntok_t *t, const char *str)
{
	int t_len = t->end - t->start;
	int res = strncmp(&json_string[t->start], str, t_len);
	return res == 0 ? str[t_len] != '\0' : res;
}

/**
 * @brief Parse a double value in a JSON object, given its key
 *
 * @param tokens The first token in the JSON file parsed by JSMN
 * @param json_string The string keeping the JSON file
 * @param obj The object in the JSON file where to look for the key
 * @param key The key to look for
 * @param result A pointer to a buffer where to store the parsed double
 * @return 0 on succedd, -1 on error
 */
static int
parse_double_by_key(c_jsmntok_t *tokens, const char *json_string, c_jsmntok_t *obj, const char *key, double *result)
{
	// sanity checks
	if(!(tokens && json_string && obj && key && result)) {
		return -1;
	}

	// parse requested double
	c_jsmntok_t *t = get_value_token_by_key(tokens, json_string, obj, key);
	if(t == NULL || parse_double_token(json_string, t, result) < 0) {
		return -1;
	}

	return 0;
}

/**
 * @brief Parse an unsigned value in a JSON object, given its key
 *
 * @param tokens The first token in the JSON file parsed by JSMN
 * @param json_string The string keeping the JSON file
 * @param obj The object in the JSON file where to look for the key
 * @param key The key to look for
 * @param result A pointer to a buffer where to store the parsed unsigned
 * @return 0 on succedd, -1 on error
 */
static int
parse_unsigned_by_key(c_jsmntok_t *tokens, const char *json_string, c_jsmntok_t *obj, const char *key, unsigned *result)
{
	// sanity checks
	if(!(tokens && json_string && obj && key && result)) {
		return -1;
	}

	// parse requested unsigned
	c_jsmntok_t *t = get_value_token_by_key(tokens, json_string, obj, key);
	if(t == NULL || parse_unsigned_token(json_string, t, result) < 0) {
		return -1;
	}

	return 0;
}

/**
 * @brief Parse a boolean value in a JSON object, given its key
 *
 * @param tokens The first token in the JSON file parsed by JSMN
 * @param json_string The string keeping the JSON file
 * @param obj The object in the JSON file where to look for the key
 * @param key The key to look for
 * @param result A pointer to a buffer where to store the parsed boolean
 * @return 0 on succedd, -1 on error
 */
static int
parse_boolean_by_key(c_jsmntok_t *tokens, const char *json_string, c_jsmntok_t *obj, const char *key, bool *result)
{
	// sanity checks
	if(!(tokens && json_string && obj && key && result)) {
		return -1;
	}

	// find relevant token
	c_jsmntok_t *t = get_value_token_by_key(tokens, json_string, obj, key);
	if(t == NULL || t->type != JSMN_PRIMITIVE) {
		return -1;
	}

	// parse the boolean value
	if(!strcmp_token(json_string, t, "false")) {
		*result = false;
	} else if(!strcmp_token(json_string, t, "true")) {
		*result = true;
	} else {
		return -1;
	}

	return 0;
}

/**
 * @brief Locate a substring of given length in a string, and make a copy
 *
 * This function allocates a buffer using malloc, to keep the copy of the string.
 * This is the buffer which is returned.
 *
 * @param src A pointer to a NULL-terminated string where to start extracting a string
 * @param nchars The number of characters (\0 excluded) to copy
 * @return A pointer to a copy of the substring
 */
static inline char *json_substr(const char *src, size_t nchars)
{
	if(!src) {
		return NULL;
	}

	char *dup = mm_alloc(nchars + 1);
	memcpy(dup, src, nchars);
	dup[nchars] = '\0';
	return dup;
}

/**
 * @brief Locate the metadata to perform reflection of a struct given its name
 *
 * @param struct_name The name of the struct
 * @return A pointer to a struct autoreconf_type_map describing how to perform reflection
 */
static struct autoconf_type_map *find_conf_map_by_name(const char *struct_name)
{
	struct autoconf_type_map *populate = NULL;
	struct autoconf_name_map *maps;
	unsigned i = 0;

	if(!struct_name) {
		return NULL;
	}

	while(true) {
		maps = &autoconf_structs[i++];
		if(strcmp(maps->struct_name, struct_name) == 0) {
			populate = maps->members;
			break;
		}
	}
	assert(populate != NULL);
	return populate;
}

/**
 * @brief Load an array from a JSON file
 *
 * This function does not handle arrays of objects. See load_json_array_of_objects() instead.
 *
 * @param arr_type The type of the elements of the array: unsigned, double, bool, string
 * @param tokens The first token in the JSON file parsed by JSMN
 * @param arr The JSMN token identifying the JSON array
 * @param json_string The JSON file
 * @return A pointer to a contiguous buffer of elements of the type specified by arr_type
 */
static void *load_json_array(unsigned arr_type, const jsmntok_t *tokens, const jsmntok_t *arr, const char *json_string)
{
	unsigned memb_size = 0;

	if(!(tokens && arr && json_string)) {
		return NULL;
	}

	switch(arr_type) {
		case AUTOCONF_ARRAY_UNSIGNED:
			memb_size = sizeof(unsigned);
			break;
		case AUTOCONF_ARRAY_DOUBLE:
			memb_size = sizeof(double);
			break;
		case AUTOCONF_ARRAY_STRING:
			memb_size = sizeof(char *);
			break;
		default:
			abort();
	}

	struct vector *v = init_vector(8, memb_size);

	struct gnt_closure_t closure = GNT_CLOSURE_INITIALIZER;
	const jsmntok_t *t_aux;

	void *element = mm_alloc(memb_size);
	if(!element) {
		abort();
	}

	for(int i = 0; i < arr->size; i++) {
		memset(element, 0, memb_size);
		t_aux = get_next_token(tokens, arr, &closure);

		switch(arr_type) {
			case AUTOCONF_ARRAY_UNSIGNED:
				parse_unsigned_token(json_string, t_aux, element);
				break;
			case AUTOCONF_ARRAY_DOUBLE:
				parse_double_token(json_string, t_aux, element);
				break;
			case AUTOCONF_ARRAY_STRING:
				*(char **) element = json_substr(&json_string[t_aux->start], t_aux->end - t_aux->start);
				break;
			default:
				abort();
		}

		push_back(v, element);
	}

	mm_free(element);
	return freeze_vector(v);
}

/**
 * @brief Load an array of objects from a JSON file
 *
 * This function allocates via malloc() the structs to keep the various objects
 *
 * @param struct_name A string representing the type of the struct
 * @param struct_size The size in bytes of the struct
 * @param tokens The first token in the JSON file parsed by JSMN
 * @param arr The JSMN token identifying the JSON array
 * @param json_string The JSON file
 * @return A pointer to a contiguous buffer of dynamically allocated structs
 */
static void *
load_json_array_of_objects(const char *struct_name, size_t struct_size, const jsmntok_t *tokens, const jsmntok_t *arr,
	const char *json_string)
{
	if(!(struct_name && tokens && arr)) {
		return NULL;
	}

	struct vector *v = init_vector(8, struct_size);

	struct autoconf_type_map *mappings = find_conf_map_by_name(struct_name);
	assert(mappings != NULL);

	struct gnt_closure_t closure = GNT_CLOSURE_INITIALIZER;
	const jsmntok_t *t_aux;

	void *element = mm_alloc(struct_size);
	if(!element) {
		abort();
	}

	for(int i = 0; i < arr->size; i++) {
		memset(element, 0, struct_size);

		t_aux = get_next_token(tokens, arr, &closure);
		assert(t_aux != NULL);

		if(load_json_object(element, struct_name, tokens, t_aux, json_string) < 0) {
			free_vector(v);
			mm_free(element);
			return NULL;
		}
		push_back(v, element);
	}

	mm_free(element);
	return freeze_vector(v);
}

/**
 * @brief Load an object into a struct
 *
 * The struct must be described by a struct autoconf_type_map generated at compile time by the ROOT-Sim compiler
 *
 * @param struct_ptr A pointer to the struct where to load the JSON object
 * @param struct_name A string representing the type of the struct
 * @param tokens The first token in the JSON file parsed by JSMN
 * @param obj The JSMN token describing the JSON object
 * @param json_string The JSON file
 * @return 0 on success, <0 on error
 */
static int load_json_object(void *struct_ptr, const char *struct_name, c_jsmntok_t *tokens, c_jsmntok_t *obj,
	const char *json_string)
{
	if(!(struct_ptr && struct_name && tokens && obj && json_string)) {
		return -1;
	}

	struct autoconf_type_map *mappings = find_conf_map_by_name(struct_name);
	assert(mappings != NULL);

	unsigned i = 0;
	char *base_address = (char *) struct_ptr;
	int ret = 0;
	c_jsmntok_t *t;

	while(true) {
		struct autoconf_type_map *curr = &mappings[i++];

		switch(curr->type) {
			case AUTOCONF_UNSIGNED:
				ret = parse_unsigned_by_key(tokens, json_string, obj, curr->member,
							    (unsigned *) (base_address + curr->offset));
				break;

			case AUTOCONF_DOUBLE:
				ret = parse_double_by_key(tokens, json_string, obj, curr->member,
							  (double *) (base_address + curr->offset));
				break;

			case AUTOCONF_BOOL:
				ret = parse_boolean_by_key(tokens, json_string, obj, curr->member,
							   (bool *) (base_address + curr->offset));
				break;

			case AUTOCONF_STRING:
				t = get_value_token_by_key(tokens, json_string, obj, curr->member);
				*(char **) (base_address + curr->offset) = json_substr(&json_string[t->start],
										       t->end - t->start);
				if((char **) (base_address + curr->offset) == NULL) {
					ret = -1;
					break;
				}
				break;

			case AUTOCONF_OBJECT:
				*(void **) (base_address + curr->offset) = mm_alloc(curr->struct_size);
				memset(*(void **) (base_address + curr->offset), 0, curr->struct_size);
				t = get_value_token_by_key(tokens, json_string, obj, curr->member);
				ret = load_json_object(*(void **) (base_address + curr->offset), curr->struct_name,
						       tokens, t, json_string);
				break;

			case AUTOCONF_ARRAY_UNSIGNED:
			case AUTOCONF_ARRAY_DOUBLE:
			case AUTOCONF_ARRAY_STRING:
				t = get_value_token_by_key(tokens, json_string, obj, curr->member);
				*(void **) (base_address + curr->offset) = load_json_array(curr->type, tokens, t,
											   json_string);
				break;

			case AUTOCONF_ARRAY_OBJECT:
				t = get_value_token_by_key(tokens, json_string, obj, curr->member);
				*(void **) (base_address + curr->offset) = load_json_array_of_objects(curr->struct_name,
												      curr->struct_size,
												      tokens, t,
												      json_string);
				break;

			case AUTOCONF_INVALID:
				return 0;

			default:
				abort();
		}

		if(ret < 0) {
			return ret;
		}
	}

	return ret;
}


/**
 * @brief Load a JSON file into a struct.
 *
 * This function requires the top-most struct to be allocated externally and passed to the function as a pointer.
 * Any other struct (or array) pointed to by this structure, even indirectly, is dynamically allocated while
 * performing the traversal of the JSON file.
 * The top-most struct must be annotated as _autoconf in the source file, to allow the ROOT-Sim compiler to
 * generate the metadata required to perform reflection.
 *
 * @param struct_ptr A pointer to the top-most struct where to load the JSON file into
 * @param json_string The JSON file
 * @return 0 on success, non-zero on error
 */
int load_json_from_string(void *struct_ptr, const char *json_string)
{
	jsmn_parser parser;
	int tokens_cnt;
	jsmntok_t *tokens = NULL; // It's safe to call free(NULL)
	int ret = 0;

	if(!(struct_ptr && json_string)) {
		return -1;
	}

	jsmn_init(&parser);
	tokens_cnt = jsmn_parse(&parser, json_string, strlen(json_string), NULL, 0);
	if(tokens_cnt <= 0) {
		return -2;
	}

	tokens = mm_alloc(sizeof(jsmntok_t) * (size_t) tokens_cnt);
	jsmn_init(&parser);
	if(jsmn_parse(&parser, json_string, strlen(json_string), tokens, tokens_cnt) != tokens_cnt) {
		ret = -3;
		goto out;
	}

	// Assume top-most element is a JSON object
	if(load_json_object(struct_ptr, "struct conf", tokens, &tokens[0], json_string) < 0) {
		ret = -4;
	}

    out:
	mm_free(tokens);
	return ret;
}


/**
 * @brief Load a JSON file into a struct.
 *
 * This function requires the top-most struct to be allocated externally and passed to the function as a pointer.
 * Any other struct (or array) pointed to by this structure, even indirectly, is dynamically allocated while
 * performing the traversal of the JSON file.
 * The top-most struct must be annotated as _autoconf in the source file, to allow the ROOT-Sim compiler to
 * generate the metadata required to perform reflection.
 * This function simply loads the content of file in a string in memory and invokes load_json_from_string().
 *
 * @param struct_ptr A pointer to the top-most struct where to load the JSON file into
 * @param file An opened FILE descriptor
 * @return 0 on success, non-zero on error
 */
int load_json_from_file(void *struct_ptr, FILE *file)
{
	long int file_len;
	char *buffer = NULL; // it's safe to call free(NULL)
	int ret = -1;

	if(!(struct_ptr && file)) {
		goto out;
	}
	if(fseek(file, 0L, SEEK_END)) {
		goto out;
	}
	file_len = ftell(file);
	if(file_len < 0) {
		goto out;
	}
	if(fseek(file, 0L, SEEK_SET)) {
		goto out;
	}

	buffer = mm_alloc(file_len + 1);
	if(!buffer) {
		goto out;
	}
	if(!fread(buffer, (size_t) file_len, 1, file)) {
		goto out;
	}

	buffer[file_len] = '\0';

	ret = load_json_from_string(struct_ptr, buffer);

    out:
	mm_free(buffer);
	return ret;
}

/**
 * @brief Load a JSON file into a struct.
 *
 * This function requires the top-most struct to be allocated externally and passed to the function as a pointer.
 * Any other struct (or array) pointed to by this structure, even indirectly, is dynamically allocated while
 * performing the traversal of the JSON file.
 * The top-most struct must be annotated as _autoconf in the source file, to allow the ROOT-Sim compiler to
 * generate the metadata required to perform reflection.
 * This function simply opens the file located at path an invokes load_json_from_file().
 *
 * @param struct_ptr A pointer to the top-most struct where to load the JSON file into
 * @param path The path to a JSON file on disk
 * @return 0 on success, non-zero on error
 */
int load_json_from_file_path(void *struct_ptr, const char *path)
{
	FILE *t_file;
	int ret = -1;

	if(!(path && struct_ptr)) {
		return -1;
	}

	t_file = fopen(path, "r");
	if(t_file == NULL) {
		return -1;
	}

	ret = load_json_from_file(struct_ptr, t_file);

	fclose(t_file);
	return ret;
}
