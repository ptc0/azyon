#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <functional>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "editor.h"
#include "input.h"
#include "plugin.h"

namespace fs = std::filesystem;
using namespace std;

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    cout << "\033[2J\033[H";
#endif
}

void drawMenuBar(const EditorState &ed, int width, const string& message = "") {
    cout << "\033[7m"; // inverted colors
    string title = ed.currentFile.empty() ? "Welcome!" : ed.currentFile;
    string pos = " Ln " + to_string(ed.cursorY) + ", Col " + to_string(ed.cursorX);
    string line = " Azyon - " + title + pos;

    if (!message.empty()) {
        line += " | " + message;
    }

    if ((int)line.size() < width) line += string(width - line.size(), ' ');
    cout << line.substr(0, width);
    cout << "\033[0m\n"; // reset colors
}

void drawBuffer(const EditorState &ed, int width, int height) {
    for (int i = 0; i < height - 2; ++i) {
        cout << "\033[K"; // clear line
        if (i < (int)ed.buffer.size()) {
            string line = ed.buffer[i];
            if ((int)line.size() > width)
                line = line.substr(0, width);
            cout << line;
        } else {
            cout << "~";
        }
        cout << "\n";
    }
}

void drawFileTree(int width, int height, int selectedIndex, const vector<fs::directory_entry> &entries) {
    for (int i = 0; i < height - 2 && i < (int)entries.size(); ++i) {
        if (i == selectedIndex) cout << "\033[7m"; // highlight
        cout << "\033[K"; // clear line
        const auto& e = entries[i];
        string prefix = e.is_directory() ? "[D] " : "    ";
        string name = e.path().filename().string();
        string line = prefix + name;
        if ((int)line.size() > width)
            line = line.substr(0, width);
        cout << line;
        cout << "\033[0m\n";
    }
}

int main(int argc, char *argv[]) {
    EditorState editor;
    initEditor(editor);

    editor.L = luaL_newstate();
    luaL_openlibs(editor.L);
    registerLuaFunctions(editor.L, &editor);
    loadLuaPlugins(editor);

    // Parse command line args
    bool debugMode = false;
    string fileToOpen = "";
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-debug") {
            debugMode = true;
        } else {
            fileToOpen = arg;
        }
    }

    if (!fileToOpen.empty()) {
        // Try to load file if exists, else start empty with that filename
        if (filesystem::exists(fileToOpen)) {
            loadFile(editor, fileToOpen);
        } else {
            editor.currentFile = fileToOpen;
            editor.buffer.clear();
            editor.buffer.push_back("");
            editor.cursorX = 0;
            editor.cursorY = 0;
        }
    }

    // Load welcome text from file
    ifstream wfile("WELCOMESPLASH");
    string welcomeText;
    if (wfile) {
        string line;
        while (getline(wfile, line)) {
            welcomeText += line + "\n";
        }
    }

    enableRawMode();

    int termWidth = 80, termHeight = 24;
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    termWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    termHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    termWidth = ws.ws_col;
    termHeight = ws.ws_row;
#endif

    bool showWelcome = editor.currentFile.empty();
    bool showFileTree = false;
    int fileTreeSelected = 0;
    vector<filesystem::directory_entry> entries;

    auto collectEntries = [&](const filesystem::path& p) {
        entries.clear();
        function<void(const filesystem::path&, int)> listDir = [&](const filesystem::path& path, int depth) {
            for (auto& entry : filesystem::directory_iterator(path)) {
                if (entry.path().filename().string()[0] == '.') continue; // Skip hidden files
                entries.push_back(entry);
                if (entry.is_directory())
                    listDir(entry.path(), depth + 1);
            }
        };
        listDir(p, 0);
    };

    if (showWelcome && welcomeText.empty()) {
        showFileTree = true;
        collectEntries(filesystem::current_path());
    }

    string saveMessage = "";
    bool showSaveMessage = false;
    auto saveMessageTime = chrono::steady_clock::now();

    while (true) {
        if (showSaveMessage) {
            auto now = chrono::steady_clock::now();
            if (chrono::duration_cast<chrono::seconds>(now - saveMessageTime).count() > 2) {
                showSaveMessage = false;
                saveMessage = "";
            }
        }

        clearScreen();
        drawMenuBar(editor, termWidth, showSaveMessage ? saveMessage : "");

        if (showWelcome) {
            if (showFileTree) {
                drawFileTree(termWidth, termHeight, fileTreeSelected, entries);
            } else {
                if (!welcomeText.empty()) {
                    cout << welcomeText << "\n";
                    cout << "Press CTRL + ENTER to open file browser, to navigate CTRL + <arrow> or ENTER\n";
                } else {
                    cout << "Splash file is missing! Please fix ASAP.\n\n";
                    cout << "Press CTRL + ENTER to open file browser\n";
                }
            }
        } else {
            drawBuffer(editor, termWidth, termHeight);
            cout << "\033[" << (editor.cursorY + 1) << ";" << (editor.cursorX + 1) << "H";
        }

        cout.flush();

        int c = readKey();

        if (c == 27) {  // ESC clears and exits
            clearScreen();
            break;
        }

        if (editor.L) {
            callLuaFunction(editor.L, "onKeyPress", c);
        }

        if (showWelcome) {
            if (showFileTree) {
                if (c == ARROW_UP && fileTreeSelected > 0) fileTreeSelected--;
                else if (c == ARROW_DOWN && fileTreeSelected + 1 < (int)entries.size()) fileTreeSelected++;
                else if (c == '\n') {
                    if (!entries.empty() && fileTreeSelected < (int)entries.size()) {
                        auto &selected = entries[fileTreeSelected];
                        if (selected.is_directory()) {
                            collectEntries(selected.path());
                            fileTreeSelected = 0;
                        } else {
                            loadFile(editor, selected.path().string());
                            showWelcome = false;
                            showFileTree = false;
                            editor.cursorX = 0;
                            editor.cursorY = 0;
                            clearScreen();
                        }
                    }
                }
            } else {
                if (c == '\n') {
                    showFileTree = true;
                    collectEntries(filesystem::current_path());
                    fileTreeSelected = 0;
                }
            }
        } else {
            if (c == ARROW_UP) moveCursor(editor, 0, -1);
            else if (c == ARROW_DOWN) moveCursor(editor, 0, 1);
            else if (c == ARROW_LEFT) moveCursor(editor, -1, 0);
            else if (c == ARROW_RIGHT) moveCursor(editor, 1, 0);
            else if (c == '\r' || c == '\n') insertNewLine(editor);
            else if (c == 127 || c == 8) deleteChar(editor);
            else if (c == 19) { // Ctrl+S
                if (!editor.currentFile.empty()) {
                    size_t bytes = saveFile(editor);
                    saveMessage = "[ Saved " + to_string(bytes) + " bytes sucessfully ]";
                    showSaveMessage = true;
                    saveMessageTime = chrono::steady_clock::now();
                }
            }
            else if (c >= 32 && c < 127) insertChar(editor, (char)c);
        }
    }

    disableRawMode();

    if (editor.L) {
        lua_close(editor.L);
        editor.L = nullptr;
    }
    
    return 0;
}