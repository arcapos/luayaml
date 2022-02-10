/*
 * Copyright (c) 2018 - 2022 Micro Systems Marc Balmer, CH-5073 Gipf-Oberfrick.
 * All rights reserved.
 *
 * This source code is the proprietary confidential property of Micro Systems
 * Marc Balmer, and is provided to licensee solely for documentation and
 * educational purposes. Reproduction, publication, or distribution in any form
 * to any party other than the licensee is strictly prohibited.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* YAML for Lua */

#include <ctype.h>
#include <lua.h>
#include <lauxlib.h>
#include <yaml.h>

#include "luayaml.h"

#ifdef DEBUG
static int verbose = 0;

static const char *events[] = {
	"YAML_NO_EVENT",
	"YAML_STREAM_START_EVENT",
	"YAML_STREAM_END_EVENT",
	"YAML_DOCUMENT_START_EVENT",
	"YAML_DOCUMENT_END_EVENT",
	"YAML_ALIAS_EVENT",
	"YAML_SCALAR_EVENT",
	"YAML_SEQUENCE_START_EVENT",
  	"YAML_SEQUENCE_END_EVENT",
	"YAML_MAPPING_START_EVENT",
	"YAML_MAPPING_END_EVENT"
};

#define event_name(event) \
	(event > 0 && event < sizeof(events)/sizeof(events[0]) ? \
	events[event] : "unknown event type")

#define dprintf(level, fmt, ...) \
	do { if (verbose >= level) fprintf(stderr, fmt, ##__VA_ARGS__); \
	} while (0)
#else
#define dprintf(level, fmt, ...)
#endif

static int parse_mapping(lua_State *, yaml_parser_t *, int);
static int parse_sequence(lua_State *, yaml_parser_t *, int);
static int parse_node(lua_State *, yaml_parser_t *, yaml_event_t, int, int);
static int parse_document(lua_State *, yaml_parser_t *, int);
static int parse_stream(lua_State *, yaml_parser_t *, int);
static int parse(lua_State *, yaml_parser_t *, int);

static int
push_boolean(lua_State *L, char *v, int l) {
	if (!strncasecmp(v, "no", l)
	    || !strncasecmp(v, "false", l)
	    || !strncasecmp(v, "off", l)) {
		lua_pushboolean(L, 0);
		return 0;
	} else if (!strncasecmp(v, "yes", l)
	    || !strncasecmp(v, "true", l)
	    || !strncasecmp(v, "on", l)) {
		lua_pushboolean(L, 1);
		return 0;
	} else
		return -1;
}

static int
push_null(lua_State *L, char *v, int l)
{
	if (!strncmp(v, "null", l) || !strncmp(v, "~", l)) {
		lua_pushnil(L);
		return 0;
	} else
		return -1;
}

static int
push_integer(lua_State *L, char *v, int l)
{
	int n = 0;

	if (*v == '+' || *v == '-')
		n++;
	for (; n < l; n++)
		if (!isdigit(v[n]))
			return -1;
	lua_pushinteger(L, atoi(v));
	return 0;
}

static int
push_hexadecimal(lua_State *L, char *v, int l)
{
	int n;

	if (l > 2) {
		if (strncmp(v, "0x", 2))
			return -1;
		for (n = 2; n < l; n++)
			if (!isxdigit(v[n]))
				return -1;
		lua_pushinteger(L, strtol(v, NULL, 16));
		return 0;
	}
	return -1;
}

static void
push_scalar(lua_State *L, char *value, int length, char *tag, int env)
{
	if (tag != NULL) {
		if (!strcmp(tag, YAML_BOOL_TAG)) {
			if (push_boolean(L, value, length) < 0)
				lua_pushnil(L);
		} else if (!strcmp(tag, YAML_INT_TAG)) {
			if (push_hexadecimal(L, value, length))
				if (push_integer(L, value, length))
					lua_pushnil(L);
		} else if (!strcmp(tag, YAML_NULL_TAG))
			lua_pushnil(L);
		else if (!strcmp(tag, YAML_FLOAT_TAG))
			lua_pushnumber(L, atof(value));
		else if (!strcmp(tag, YAML_STR_TAG))
			lua_pushlstring(L, value, length);
		else if (!strcmp(tag, LUAYAML_FILE_TAG)) {
			luaL_loadfile(L, value);
			if (env > 0) {
				lua_pushvalue(L, env);
				lua_setupvalue(L, -2, 1);
			}
		} else if (!strcmp(tag, LUAYAML_FUNCTION_TAG)) {
			luaL_loadstring(L, value);
			if (env > 0) {
				lua_pushvalue(L, env);
				lua_setupvalue(L, -2, 1);
			}
		} else if (!strcmp(tag, LUAYAML_CALL_TAG)) {
			luaL_loadstring(L, value);
			if (env > 0) {
				lua_pushvalue(L, env);
				lua_setupvalue(L, -2, 1);
			}
			lua_call(L, 0, 1);
		} else if (!strcmp(tag, LUAYAML_CALLFILE_TAG)) {
			luaL_loadfile(L, value);
			if (env > 0) {
				lua_pushvalue(L, env);
				lua_setupvalue(L, -2, 1);
			}
			lua_call(L, 0, 1);
		} else
			lua_pushlstring(L, value, length);
	} else {
		if (!push_boolean(L, value, length))
			return;
		if (!push_null(L, value, length))
			return;
		if (!push_integer(L, value, length))
			return;
		if (!push_hexadecimal(L, value, length))
			return;
		lua_pushlstring(L, value, length);
	}
}

static int
parse_node(lua_State *L, yaml_parser_t *parser, yaml_event_t event, int value,
    int env)
{
	int rv = 0;

	lua_checkstack(L, 1);

	/* Process event */
	switch (event.type) {
	case YAML_ALIAS_EVENT:
		break;
	case YAML_SCALAR_EVENT:
		if (value)
			push_scalar(L, (char *)event.data.scalar.value,
				event.data.scalar.length,
				(char *)event.data.scalar.tag, env);
		else
			lua_pushlstring(L,
				(char *)event.data.scalar.value,
				event.data.scalar.length);
		dprintf(1, "value: %s", event.data.scalar.value);
		if (event.data.scalar.tag)
			dprintf(1, "tag: %s\n", event.data.scalar.tag);
		dprintf(1, "\n");
		break;
	case YAML_SEQUENCE_START_EVENT:
		lua_newtable(L);
		parse_sequence(L, parser, env);
		break;
	case YAML_MAPPING_START_EVENT:
		lua_newtable(L);
		parse_mapping(L, parser, env);
		break;
	default:
		return luaL_error(L, "parse_node: unexpected YAML event %d",
			event.type);
	}
	return rv;
}

static int
parse_mapping(lua_State *L, yaml_parser_t *parser, int env)
{
	yaml_event_t event;
	int done = 0;
	int value = 0;

	while (!done) {
		if (!yaml_parser_parse(parser, &event))
			return -1;

		dprintf(1, "%s\n", event_name(event.type));

		if (event.type == YAML_MAPPING_END_EVENT)
			done = 1;
		else {
			parse_node(L, parser, event, value, env);
			if (value)
				lua_settable(L, -3);
			value = 1 - value;
		}
		yaml_event_delete(&event);
	}
	return 0;
}

static int
parse_sequence(lua_State *L, yaml_parser_t *parser, int env)
{
	yaml_event_t event;
	int done = 0;
	int sequence = 1;

	lua_checkstack(L, 2);

	while (!done) {
		if (!yaml_parser_parse(parser, &event))
			return -1;

		dprintf(1, "%s\n", event_name(event.type));

		if (event.type == YAML_SEQUENCE_END_EVENT)
			done = 1;
		else {
			lua_pushinteger(L, sequence++);
			parse_node(L, parser, event, 0, env);
			lua_settable(L, -3);
		}
		yaml_event_delete(&event);
	}
	return 0;
}

static int
parse_document(lua_State *L, yaml_parser_t *parser, int env)
{
	yaml_event_t event;
	int done = 0;

	while (!done) {
		if (!yaml_parser_parse(parser, &event))
			return -1;

		dprintf(1, "%s\n", event_name(event.type));

		/* Process event */
		if (event.type == YAML_DOCUMENT_END_EVENT)
			done = 1;
		else
			parse_node(L, parser, event, 0, env);

		yaml_event_delete(&event);
	}
	return 0;
}

static int
parse_stream(lua_State *L, yaml_parser_t *parser, int env)
{
	yaml_event_t event;
	int done = 0;

	while (!done) {
		if (!yaml_parser_parse(parser, &event))
			return -1;

		dprintf(1, "%s\n", event_name(event.type));

		/* Process event */
		switch (event.type) {
		case YAML_DOCUMENT_START_EVENT:
			lua_checkstack(L, 1);
			lua_newtable(L);
			parse_document(L, parser, env);
			break;
		case YAML_STREAM_END_EVENT:
			done = 1;
			break;
		default:
			return luaL_error(L, "parse_stream: unexpected YAML "
			    "event %d", event.type);
		}
		yaml_event_delete(&event);
	}
	return 0;
}

static int
parse(lua_State *L, yaml_parser_t *parser, int env)
{
	yaml_event_t event;
	int done = 0;

	while (!done) {
		if (!yaml_parser_parse(parser, &event))
			return -1;

		dprintf(1, "%s\n", event_name(event.type));

		/* Process event */
		switch (event.type) {
		case YAML_STREAM_START_EVENT:
			parse_stream(L, parser, env);
			done = 1;
			break;
		default:
			return luaL_error(L, "parse: unexpected YAML event %d",
				event.type);
		}
		yaml_event_delete(&event);
	}
	return 0;
}

static int
parseString(lua_State *L)
{
	yaml_parser_t parser;
	const unsigned char *input;
	size_t len;
	int env = -1;

	input = (const unsigned char *)luaL_checklstring(L, 1, &len);
	if (lua_gettop(L) > 1)
		env = 2;

	yaml_parser_initialize(&parser);
	yaml_parser_set_input_string(&parser, input, len);

	if (parse(L, &parser, env))
		lua_pushnil(L);

	yaml_parser_delete(&parser);
	return 1;
}

static int
parseFile(lua_State *L)
{
	yaml_parser_t parser;
	const char *fnam;
	FILE *input;
	int env = -1;

	fnam = luaL_checkstring(L, 1);
	if (lua_gettop(L) > 1)
		env = 2;

	input = fopen(fnam, "rb");
	if (input == NULL)
		return luaL_error(L, "can't open '%s'", fnam);

	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, input);

	if (parse(L, &parser, env))
		lua_pushnil(L);

	yaml_parser_delete(&parser);
	return 1;
}

#ifdef DEBUG
static int
verbosity(lua_State *L)
{
	verbose = luaL_checkinteger(L, 1);
	return 0;
}
#endif

int
luaopen_yaml(lua_State *L)
{
	struct luaL_Reg luayaml[] = {
		{ "parse",	parseString },
		{ "parsefile",	parseFile },
#ifdef DEBUG
		{ "verbosity",	verbosity },
#endif
		{ NULL, NULL }
	};

	luaL_newlib(L, luayaml);
	lua_pushliteral(L, "_COPYRIGHT");
	lua_pushliteral(L, "Copyright (C) 2018 - 2022 "
	    "micro systems marc balmer");
	lua_settable(L, -3);
	lua_pushliteral(L, "_DESCRIPTION");
#ifdef DEBUG
	lua_pushliteral(L, "YAML for Lua (with debug functions)");
#else
	lua_pushliteral(L, "YAML for Lua");
#endif
	lua_settable(L, -3);
	lua_pushliteral(L, "_VERSION");
	lua_pushliteral(L, "yaml 1.1.1");
	lua_settable(L, -3);
	return 1;
}
