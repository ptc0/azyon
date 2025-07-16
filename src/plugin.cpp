#include "plugin.h"
#include <filesystem>
#include <iostream>

static EditorState* globalEd = nullptr; // To be set from main

// C++ functions exposed to Lua

static int lua_getCursorX(lua_State* L) {
    lua_pushinteger(L, globalEd->cursorX);
    return 1;
}

static int lua_setCursorX(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    globalEd->cursorX = x;
    return 0;
}

static int lua_getCursorY(lua_State* L) {
    lua_pushinteger(L, globalEd->cursorY);
    return 1;
}

static int lua_setCursorY(lua_State* L) {
    int y = luaL_checkinteger(L, 1);
    globalEd->cursorY = y;
    return 0;
}

static int lua_getCurrentFile(lua_State* L) {
    lua_pushstring(L, globalEd->currentFile.c_str());
    return 1;
}

void registerLuaFunctions(lua_State* L, EditorState* ed) {
    globalEd = ed;

    lua_register(L, "getCursorX", lua_getCursorX);
    lua_register(L, "setCursorX", lua_setCursorX);
    lua_register(L, "getCursorY", lua_getCursorY);
    lua_register(L, "setCursorY", lua_setCursorY);
    lua_register(L, "getCurrentFile", lua_getCurrentFile);
}

void loadLuaPlugins(EditorState& ed) {
    lua_State* L = ed.L;
    if (!L) return;

    for (const auto& file : std::filesystem::directory_iterator("plugins")) {
        if (file.path().extension() == ".lua") {
            if (luaL_dofile(L, file.path().string().c_str()) != LUA_OK) {
                std::cerr << "[Lua Error] " << lua_tostring(L, -1) << "\n";
                lua_pop(L, 1);
            } else {
                std::cout << "[Lua Plugin] Loaded: " << file.path().filename().string() << "\n";
            }
        }
    }
}

void callLuaFunction(lua_State* L, const std::string& funcName, int key) {
    lua_getglobal(L, funcName.c_str());
    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, key);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            std::cerr << "[Lua Error in " << funcName << "] " << lua_tostring(L, -1) << "\n";
            lua_pop(L, 1);
        }
    } else {
        lua_pop(L, 1);
    }
}