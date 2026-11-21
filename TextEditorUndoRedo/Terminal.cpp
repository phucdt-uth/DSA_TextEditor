// ============================================================
// Terminal.cpp
// Windows : <windows.h>, <conio.h> (_getch)
// Linux/Mac: <termios.h>, <unistd.h>, <sys/ioctl.h>, <sys/select.h>
// ============================================================
#include "Terminal.h"

#ifdef _WIN32
// ------------------------- WINDOWS -------------------------
#include <windows.h>
#include <conio.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

Terminal::Terminal() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    oldOutMode_ = mode;
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);  // bật mã ANSI (Windows 10+)
    oldCodePage_ = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);                                    // hiển thị tiếng Việt UTF-8
}
Terminal::~Terminal() {
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), oldOutMode_);
    SetConsoleOutputCP(oldCodePage_);
}

int Terminal::readKey() {
    int c = _getch();
    if (c == 0 || c == 224) {                 // phím đặc biệt: 2 byte
        switch (_getch()) {
        case 72: return K_UP;    case 80: return K_DOWN;
        case 75: return K_LEFT;  case 77: return K_RIGHT;
        case 71: return K_HOME;  case 79: return K_END;
        case 73: return K_PGUP;  case 81: return K_PGDN;
        case 83: return K_DELETE;
        default: return K_NONE;
        }
    }
    if (c == 13 || c == 10) return K_ENTER;
    if (c == 8 || c == 127) return K_BACKSPACE;
    if (c == 9) return K_TAB;
    if (c == 27) return K_ESC;
    return c;
}

void Terminal::getSize(int& rows, int& cols) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        cols = info.srWindow.Right - info.srWindow.Left + 1;
        rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    }
    else { rows = 24; cols = 80; }
}

void Terminal::write(const std::string& s) {
    DWORD written = 0;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s.data(), (DWORD)s.size(), &written, NULL);
}

#else
// ------------------------- LINUX / MAC -------------------------
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>

Terminal::Terminal() {
    tcgetattr(0, &saved_);
    termios raw = saved_;
    raw.c_iflag &= ~(IXON | ICRNL | INLCR | ISTRIP | BRKINT);  // cho phép bắt Ctrl+S/Q, Enter = 13
    raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);           // đọc từng phím, không tự hiện, bắt Ctrl+C/Z
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(0, TCSAFLUSH, &raw);
}
Terminal::~Terminal() { tcsetattr(0, TCSAFLUSH, &saved_); }

// Chờ có dữ liệu trong ms mili giây (dùng để phân biệt phím ESC với chuỗi mũi tên)
static bool waitInput(int ms) {
    fd_set fds; FD_ZERO(&fds); FD_SET(0, &fds);
    timeval tv; tv.tv_sec = 0; tv.tv_usec = ms * 1000;
    return select(1, &fds, NULL, NULL, &tv) > 0;
}
static bool readByte(unsigned char& c) { return waitInput(50) && read(0, &c, 1) == 1; }

int Terminal::readKey() {
    unsigned char c;
    if (read(0, &c, 1) != 1) return K_NONE;       // bị ngắt (vd: đổi kích thước cửa sổ)
    if (c == 13 || c == 10) return K_ENTER;
    if (c == 127 || c == 8) return K_BACKSPACE;
    if (c == 9) return K_TAB;
    if (c != 27) return c;

    // c == ESC: có thể là phím ESC thật hoặc chuỗi ESC [ A ...
    unsigned char s1;
    if (!readByte(s1)) return K_ESC;
    if (s1 != '[' && s1 != 'O') return K_ESC;
    int num = 0;
    unsigned char ch;
    while (readByte(ch)) {
        if (ch >= '0' && ch <= '9') { num = num * 10 + (ch - '0'); continue; }
        switch (ch) {
        case 'A': return K_UP;    case 'B': return K_DOWN;
        case 'C': return K_RIGHT; case 'D': return K_LEFT;
        case 'H': return K_HOME;  case 'F': return K_END;
        case '~':
            if (num == 1 || num == 7) return K_HOME;
            if (num == 4 || num == 8) return K_END;
            if (num == 3) return K_DELETE;
            if (num == 5) return K_PGUP;
            if (num == 6) return K_PGDN;
            return K_NONE;
        default: return K_NONE;
        }
    }
    return K_NONE;
}

void Terminal::getSize(int& rows, int& cols) {
    winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) { rows = ws.ws_row; cols = ws.ws_col; }
    else { rows = 24; cols = 80; }
}

void Terminal::write(const std::string& s) {
    size_t done = 0;
    while (done < s.size()) {
        ssize_t n = ::write(1, s.data() + done, s.size() - done);
        if (n <= 0) break;
        done += (size_t)n;
    }
}
#endif