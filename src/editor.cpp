#include "editor.h"
#include <fstream>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}


void initEditor(EditorState& ed) {
    ed.buffer.clear();
    ed.cursorX = 0;
    ed.cursorY = 0;
}

void loadFile(EditorState& ed, const std::string& filename) {
    ed.buffer.clear();
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        ed.buffer.push_back(line);
    }
    ed.currentFile = filename;
    ed.cursorX = 0;
    ed.cursorY = 0;
}

void insertChar(EditorState& ed, char c) {
    if (ed.cursorY >= (int)ed.buffer.size()) {
        ed.buffer.resize(ed.cursorY + 1);
    }
    std::string& line = ed.buffer[ed.cursorY];
    if (ed.cursorX > (int)line.size()) ed.cursorX = line.size();
    line.insert(line.begin() + ed.cursorX, c);
    ed.cursorX++;
}

void deleteChar(EditorState& ed) {
    if (ed.cursorY < (int)ed.buffer.size()) {
        std::string& line = ed.buffer[ed.cursorY];
        if (ed.cursorX > 0 && ed.cursorX <= (int)line.size()) {
            line.erase(ed.cursorX - 1, 1);
            ed.cursorX--;
        }
    }
}

void insertNewLine(EditorState& ed) {
    if (ed.cursorY >= (int)ed.buffer.size()) {
        ed.buffer.resize(ed.cursorY + 1);
    }
    std::string& line = ed.buffer[ed.cursorY];
    std::string newLine = line.substr(ed.cursorX);
    line = line.substr(0, ed.cursorX);
    ed.buffer.insert(ed.buffer.begin() + ed.cursorY + 1, newLine);
    ed.cursorY++;
    ed.cursorX = 0;
}

void moveCursor(EditorState& ed, int dx, int dy) {
    ed.cursorY += dy;
    if (ed.cursorY < 0) ed.cursorY = 0;
    if (ed.cursorY >= (int)ed.buffer.size()) ed.cursorY = ed.buffer.size() - 1;
    if (ed.cursorY < 0) ed.cursorY = 0;

    std::string& line = ed.buffer.empty() ? *(new std::string()) : ed.buffer[ed.cursorY];
    ed.cursorX += dx;
    if (ed.cursorX < 0) ed.cursorX = 0;
    if (ed.cursorX > (int)line.size()) ed.cursorX = line.size();

    // SCROLLING:
    const int screenRows = 24 - 2;  // Adjust if terminal height changes dynamically
    if (ed.cursorY < ed.rowOffset) {
        ed.rowOffset = ed.cursorY;
    } else if (ed.cursorY >= ed.rowOffset + screenRows) {
        ed.rowOffset = ed.cursorY - screenRows + 1;
    }
}

size_t saveFile(const EditorState& ed) {
    std::ofstream file(ed.currentFile);
    size_t bytesWritten = 0;
    for (size_t i = 0; i < ed.buffer.size(); i++) {
        file << ed.buffer[i];
        bytesWritten += ed.buffer[i].size();
        if (i + 1 < ed.buffer.size()) {
            file << "\n";
            bytesWritten++; // newline byte
        }
    }
    file.close();
    return bytesWritten;
}