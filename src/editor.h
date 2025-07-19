#ifndef EDITOR_H
#define EDITOR_H

#include <string>
#include <vector>
#include <lua.hpp>

struct EditorState {
    std::vector<std::string> buffer;
    int cursorX;
    int cursorY;
    int rowOffset = 0;
    std::string currentFile;

    lua_State* L;  // Add this for Lua state pointer
};

void initEditor(EditorState& ed);
void loadFile(EditorState& ed, const std::string& filename);
void insertChar(EditorState& ed, char c);
void deleteChar(EditorState& ed);
void insertNewLine(EditorState& ed);
void moveCursor(EditorState& ed, int dx, int dy);
size_t saveFile(const EditorState& ed);

#endif