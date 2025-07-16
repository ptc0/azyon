#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <iostream>
#include "input.h"

// Windows variables and functions
static DWORD originalMode;
static HANDLE hStdin;
static HANDLE hStdout;
static DWORD originalOutMode;

void enableRawMode() {
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &originalMode);

    DWORD rawMode = originalMode;
    rawMode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);

    SetConsoleMode(hStdin, rawMode);

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(hStdout, &originalOutMode);
    DWORD outMode = originalOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hStdout, outMode);
}

void disableRawMode() {
    SetConsoleMode(hStdin, originalMode);
    SetConsoleMode(hStdout, originalOutMode);
}

int readKey() {
    int c = _getch();

    if (c == 0 || c == 224) {
        int special = _getch();
        switch (special) {
            case 72: return ARROW_UP;
            case 80: return ARROW_DOWN;
            case 75: return ARROW_LEFT;
            case 77: return ARROW_RIGHT;
            case 71: return 'H'; // Home
            case 79: return 'E'; // End
            case 83: return 127; // Delete
            default: return 0;
        }
    }
    return c;
}

#else // Unix / Linux / macOS

#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string>

static struct termios orig_termios;

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;

    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_iflag &= ~(IXON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

int readKey() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\033') {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\033';
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\033';

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                }
            }
            return '\033';
        } else {
            return c;
        }
    }
    return -1;
}

// Example helper function to print colored text (optional)
void printColor(const std::string& text, const std::string& colorCode) {
    std::cout << "\033[" << colorCode << "m" << text << "\033[0m";
}

#endif
