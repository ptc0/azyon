#include "highlight.h"
#include <string>
#include <iostream>

void renderLineWithHighlight(const std::string& line) {
    for (char c : line) {
        if (isdigit(c)) {
            std::cout << "\033[33m" << c << "\033[0m";
        } else {
            std::cout << c;
        }
    }
}
