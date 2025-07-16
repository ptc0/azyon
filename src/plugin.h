#ifndef PLUGIN_H
#define PLUGIN_H

#include "editor.h"
#include <lua.hpp>
#include <string>

void registerLuaFunctions(lua_State* L, EditorState* ed);
void loadLuaPlugins(EditorState& ed);
void callLuaFunction(lua_State* L, const std::string& funcName, int key);

#endif