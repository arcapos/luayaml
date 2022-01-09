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

#include <lua.h>
#include <lauxlib.h>
#include <yaml.h>

#include "luayaml.h"

static int verbose = 0;

#define dprintf(level, fmt, ...) \
	do { if (verbose >= level) fprintf(stderr, fmt, ##__VA_ARGS__); \
	} while (0)

static int
parse(lua_State *L, yaml_parser_t parser)
{
	yaml_event_t event;
	int done = 0;
	int document = 0;
	int mapping = 0;
	int value = 0;
	int sequence = 0;

	while (!done) {
		if (!yaml_parser_parse(&parser, &event))
			return -1;

		/* Process event */
		switch (event.type) {
			case YAML_STREAM_START_EVENT:
				dprintf(1, "YAML_STREAM_START_EVENT\n");
				break;
			case YAML_DOCUMENT_START_EVENT:
				dprintf(1, "YAML_DOCUMENT_START_EVENT\n");
				lua_checkstack(L, 1);
				lua_newtable(L);
				document = 1;
				break;
			case YAML_DOCUMENT_END_EVENT:
				dprintf(1, "YAML_DOCUMENT_END_EVENT\n");
				break;
			case YAML_ALIAS_EVENT:
				dprintf(1, "YAML_ALIAS_EVENT\n");
				break;
			case YAML_SCALAR_EVENT:
				dprintf(1, "YAML_SCALAR_EVENT\t%c%c%c%c\t%s\n",
				    document ? 'D' : '-',
				    mapping ? 'M' : '-',
				    value ? 'V' : '-',
				    sequence ? 'S' : '-',
				    event.data.scalar.value);
				lua_checkstack(L, 2);

				if (sequence)
					lua_pushinteger(L, sequence++);

				lua_pushlstring(L,
					event.data.scalar.value,
					event.data.scalar.length);

				if (sequence)
					lua_settable(L, -3);
				else {
					if (value) {
						lua_settable(L, -3);
						value = 0;
					} else
						value = 1;
				}
				break;
			case YAML_SEQUENCE_START_EVENT:
				dprintf(1, "YAML_SEQUENCE_START_EVENT\t%c%c%c%c\n",
				    document ? 'D' : '-',
				    mapping ? 'M' : '-',
				    value ? 'V' : '-',
				    sequence ? 'S' : '-');
				sequence = 1;
				value = 0;
				lua_checkstack(L, 1);
				lua_newtable(L);
				break;
			case YAML_SEQUENCE_END_EVENT:
				dprintf(1, "YAML_SEQUENCE_END_EVENT\n");
				lua_settable(L, -3);
				sequence = 0;
				break;
			case YAML_MAPPING_START_EVENT:
				dprintf(1, "YAML_MAPPING_START_EVENT\t%c%c%c%c\n",
				    document ? 'D' : '-',
				    mapping ? 'M' : '-',
				    value ? 'V' : '-',
				    sequence ? 'S' : '-');
				if (!document) {
					lua_checkstack(L, 1);
					lua_newtable(L);
					mapping++;
				} else
					document = 0;
				value = 0;
				break;
			case YAML_MAPPING_END_EVENT:
				dprintf(1, "YAML_MAPPING_END_EVENT\n");
				if (mapping) {
					lua_settable(L, -3);
					mapping--;
				}
				break;
			case YAML_STREAM_END_EVENT:
				dprintf(1, "YAML_STREAM_END_EVENT\n");
				done = 1;
		}

		yaml_event_delete(&event);
	}
	return 0;
}

static int
parseString(lua_State *L)
{
	yaml_parser_t parser;
	const char *input;
	size_t len;

	input = luaL_checklstring(L, 1, &len);

	yaml_parser_initialize(&parser);
	yaml_parser_set_input_string(&parser, input, len);

	if (parse(L, parser))
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

	fnam = luaL_checkstring(L, 1);
	input = fopen(fnam, "rb");
	if (input == NULL)
		return luaL_error(L, "can't open '%s'", fnam);

	yaml_parser_initialize(&parser);
	yaml_parser_set_input_file(&parser, input);

	if (parse(L, parser))
		lua_pushnil(L);

	yaml_parser_delete(&parser);
	return 1;
}

static int
verbosity(lua_State *L)
{
	verbose = luaL_checkinteger(L, 1);
	return 0;
}

int
luaopen_yaml(lua_State *L)
{
	struct luaL_Reg luayaml[] = {
		{ "parse",	parseString },
		{ "parsefile",	parseFile },
		{ "verbosity",	verbosity },
		{ NULL, NULL }
	};

	luaL_newlib(L, luayaml);
	lua_pushliteral(L, "_COPYRIGHT");
	lua_pushliteral(L, "Copyright (C) 2018 - 2022 "
	    "micro systems marc balmer");
	lua_settable(L, -3);
	lua_pushliteral(L, "_DESCRIPTION");
	lua_pushliteral(L, "YAML for Lua");
	lua_settable(L, -3);
	lua_pushliteral(L, "_VERSION");
	lua_pushliteral(L, "yaml 1.0.1");
	lua_settable(L, -3);
	return 1;
}
