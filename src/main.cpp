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
    cout << "\033[7m";
    string title = ed.currentFile.empty() ? "Welcome!" : ed.currentFile;
    string pos = " Ln " + to_string(ed.cursorY) + ", Col " + to_string(ed.cursorX);
    string line = " Azyon - " + title + pos;

    if (!message.empty()) {
        line += " | " + message;
    }

    if ((int)line.size() < width) line += string(width - line.size(), ' ');
    cout << line.substr(0, width);
    cout << "\033[0m\n";
}

void drawBuffer(const EditorState &ed, int width, int height) {
    int screenRows = height - 2;

    for (int i = 0; i < screenRows; ++i) {
        int fileRow = i + ed.rowOffset;
        cout << "\033[K";
        if (fileRow < (int)ed.buffer.size()) {
            string line = ed.buffer[fileRow];
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
    const int margin = 2; // left padding
    for (int i = 0; i < height - 2 && i < (int)entries.size(); ++i) {
        if (i == selectedIndex) cout << "\033[7m";
        cout << "\033[K";

        const auto& e = entries[i];
        string prefix;
        string name;

        if (i == 0 && e.path().filename() != e.path()) {
            // ".." entry
            prefix = " 🗀 ";
            name = "..";
        } else if (e.is_directory()) {
            prefix = " 🗀 ";
            name = e.path().filename().string();
        } else {
            prefix = " 📄 ";
            name = e.path().filename().string();
        }

        string line = string(margin, ' ') + prefix + name;

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
    fs::path currentDirectory = fs::current_path();

    auto collectEntries = [&](const filesystem::path& path) {
        entries.clear();
        vector<filesystem::directory_entry> tempEntries;

        // Add ".." manually as first entry
        if (path.has_parent_path()) {
            entries.push_back(fs::directory_entry(path.parent_path()));
        }

        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                tempEntries.push_back(entry);
            }

            sort(tempEntries.begin(), tempEntries.end(), [](const auto& a, const auto& b) {
                if (a.is_directory() != b.is_directory())
                    return a.is_directory();
                return a.path().filename().string() < b.path().filename().string();
            });

            entries.insert(entries.end(), tempEntries.begin(), tempEntries.end());
        } catch (const filesystem::filesystem_error& e) {
            if (debugMode) {
                cerr << "[Directory Error] " << e.what() << "\n";
            }
        }
    };

    if (showWelcome && welcomeText.empty()) {
        showFileTree = true;
        collectEntries(currentDirectory);
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
                    vector<string> lines;
                    string line;
                    istringstream stream(welcomeText);
                    size_t maxLineLength = 0;

                    while (getline(stream, line)) {
                        lines.push_back(line);
                        if (line.length() > maxLineLength)
                            maxLineLength = line.length();
                    }

                    string instructions = "Press CTRL + ENTER to open file browser, to navigate CTRL + <arrow> or ENTER";
                    lines.push_back("");
                    lines.push_back(instructions);

                    int totalLines = lines.size();
                    int verticalPadding = max(0, (termHeight - totalLines) / 2);

                    for (int i = 0; i < verticalPadding; ++i)
                        cout << "\n";

                    for (const auto& l : lines) {
                        int padding = max(0, (termWidth - (int)l.length()) / 2);
                        cout << string(padding, ' ') << l << "\n";
                    }
                } else {
                    cout << "Splash file is missing! Please fix ASAP.\n\n";
                    cout << "Press CTRL + ENTER to open file browser\n";
                }
            }
        } else {
            drawBuffer(editor, termWidth, termHeight);
            cout << "\033[" << (editor.cursorY + 2) << ";" << (editor.cursorX + 1) << "H";
        }

        cout.flush();

        int c = readKey();

        if (c == 27) {  // ESC
            if (!showWelcome) {
                cout << "\nDo you want to save changes before returning to file browser? (y/n): ";
                cout.flush();
                char response;
                cin >> response;
                if (response == 'y' || response == 'Y') {
                    if (!editor.currentFile.empty()) {
                        size_t bytes = saveFile(editor);
                        saveMessage = "[ Saved " + to_string(bytes) + " bytes successfully ]";
                        showSaveMessage = true;
                        saveMessageTime = chrono::steady_clock::now();
                    }
                }
                showWelcome = true;
                showFileTree = true;
                collectEntries(currentDirectory);
                fileTreeSelected = 0;
                continue;
            } else {
                continue; // ESC in file browser does nothing
            }
        }

        if (editor.L) {
            callLuaFunction(editor.L, "onKeyPress", c);
        }

        if (showWelcome && showFileTree) {
            switch (c) {
                case ARROW_UP:
                    if (fileTreeSelected > 0) fileTreeSelected--;
                    break;
                case ARROW_DOWN:
                    if (fileTreeSelected + 1 < (int)entries.size()) fileTreeSelected++;
                    break;
                case '\n':
                    if (!entries.empty() && fileTreeSelected < (int)entries.size()) {
                        const auto& selected = entries[fileTreeSelected];
                        if (selected.is_directory()) {
                            currentDirectory = selected.path();
                            collectEntries(currentDirectory);
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
                    break;
                default:
                    break; // Ignore other keys (e.g. Ctrl+Arrow)
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
                    saveMessage = "[ Saved " + to_string(bytes) + " bytes successfully ]";
                    showSaveMessage = true;
                    saveMessageTime = chrono::steady_clock::now();
                }
            } else if (c >= 32 && c < 127) insertChar(editor, (char)c);
        }
    }

    disableRawMode();

    if (editor.L) {
        lua_close(editor.L);
        editor.L = nullptr;
    }

    return 0;
}