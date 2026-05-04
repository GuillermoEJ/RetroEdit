#pragma once
#include <string>

namespace retro {

class Terminal {
public:
    static void enableRawMode();
    static void disableRawMode();
    static int readKey();
    static void getSize(int& rows, int& cols);
    static void clear();
    static void moveCursor(int row, int col);
    static void setColor(int fg, int bg = -1);
    static void resetColor();
    static void write(const std::string& s);
    static void flush();
};

enum Color {
    BLACK = 0, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
    BRIGHT_BLACK = 8, BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW,
    BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE
};

enum Key {
    KEY_ESCAPE = 27,
    KEY_ENTER = 13,
    KEY_BACKSPACE = 127,
    KEY_TAB = 9,
    KEY_ARROW_UP = 1000,
    KEY_ARROW_DOWN,
    KEY_ARROW_LEFT,
    KEY_ARROW_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_DELETE,
};

}
