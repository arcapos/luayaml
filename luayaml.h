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

#ifndef __LUA_YAML__
#define __LUA_YAML__

#define LUAYAML_LUA_CALL_TAG		"!Lua/call"
#define LUAYAML_LUA_CALLFILE_TAG	"!Lua/callfile"
#define LUAYAML_LUA_FILE_TAG		"!Lua/file"
#define LUAYAML_LUA_FUNCTION_TAG	"!Lua/function"

#define LUAYAML_FILE_TAG		"!file"

extern int luaopen_yaml(lua_State *L);

#endif /* __LUA_YAML__ */
