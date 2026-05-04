#include "terminal.h"
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

namespace retro {

#ifndef _WIN32
static struct termios origTermios;
#endif

void Terminal::enableRawMode() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hIn, &mode);
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    SetConsoleMode(hIn, mode);
#else
    tcgetattr(STDIN_FILENO, &origTermios);
    struct termios raw = origTermios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif
}

void Terminal::disableRawMode() {
#ifdef _WIN32
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hIn, &mode);
    mode |= (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    SetConsoleMode(hIn, mode);
#else
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
#endif
}

int Terminal::readKey() {
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 0xE0) {
        int ext = _getch();
        switch (ext) {
            case 72: return KEY_ARROW_UP;
            case 80: return KEY_ARROW_DOWN;
            case 75: return KEY_ARROW_LEFT;
            case 77: return KEY_ARROW_RIGHT;
            case 71: return KEY_HOME;
            case 79: return KEY_END;
            case 73: return KEY_PAGE_UP;
            case 81: return KEY_PAGE_DOWN;
            case 83: return KEY_DELETE;
        }
        return 0;
    }
    if (c == 8) return KEY_BACKSPACE;
    if (c == 13) return KEY_ENTER;
    return c;
#else
    char c;
    int nread = read(STDIN_FILENO, &c, 1);
    if (nread <= 0) return 0;

    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KEY_ESCAPE;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESCAPE;
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return KEY_ESCAPE;
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return KEY_HOME;
                        case '3': return KEY_DELETE;
                        case '4': return KEY_END;
                        case '5': return KEY_PAGE_UP;
                        case '6': return KEY_PAGE_DOWN;
                        case '7': return KEY_HOME;
                        case '8': return KEY_END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return KEY_ARROW_UP;
                    case 'B': return KEY_ARROW_DOWN;
                    case 'C': return KEY_ARROW_RIGHT;
                    case 'D': return KEY_ARROW_LEFT;
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                }
            }
        }
        return KEY_ESCAPE;
    }
    if (c == 127) return KEY_BACKSPACE;
    if (c == '\r') return KEY_ENTER;
    return static_cast<int>(c);
#endif
}

void Terminal::getSize(int& rows, int& cols) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    rows = ws.ws_row;
    cols = ws.ws_col;
#endif
}

void Terminal::clear() {
    write("\x1b[2J\x1b[H");
}

void Terminal::moveCursor(int row, int col) {
    write("\x1b[" + std::to_string(row + 1) + ";" + std::to_string(col + 1) + "H");
}

void Terminal::setColor(int fg, int bg) {
    if (fg >= 0 && fg < 8)
        write("\x1b[" + std::to_string(30 + fg) + "m");
    else if (fg >= 8 && fg < 16)
        write("\x1b[" + std::to_string(90 + fg - 8) + "m");
    if (bg >= 0 && bg < 8)
        write("\x1b[" + std::to_string(40 + bg) + "m");
    else if (bg >= 8 && bg < 16)
        write("\x1b[" + std::to_string(100 + bg - 8) + "m");
}

void Terminal::resetColor() {
    write("\x1b[0m");
}

void Terminal::write(const std::string& s) {
    std::fputs(s.c_str(), stdout);
}

void Terminal::flush() {
    std::fflush(stdout);
}

}
