// ============================================================
// Terminal.h - Lớp trừu tượng hóa console (Windows + Linux/macOS)
// Việc của lớp: đọc phím (không cần Enter), lấy kích thước, ghi chuỗi.
// Hiển thị dùng mã ANSI (\x1b[...) nên mã UI chạy giống nhau trên mọi hệ điều hành.
// ============================================================
#pragma once
#include <string>
#ifndef _WIN32
#include <termios.h>
#endif

// Mã phím. Ký tự in được = mã ASCII 32..126. Ctrl+A..Z = 1..26.
enum Key {
    K_NONE = 0,
    K_ENTER = 1000, K_BACKSPACE, K_TAB, K_ESC,
    K_UP, K_DOWN, K_LEFT, K_RIGHT,
    K_HOME, K_END, K_PGUP, K_PGDN, K_DELETE
};

class Terminal {
public:
    Terminal();    // bật chế độ đọc phím thô (raw mode)
    ~Terminal();   // trả console về trạng thái cũ
    int readKey();                          // đọc 1 phím (chờ đến khi có)
    void getSize(int& rows, int& cols);     // kích thước cửa sổ console
    void write(const std::string& s);       // ghi chuỗi ra màn hình
private:
#ifdef _WIN32
    unsigned long oldOutMode_;   // chế độ output cũ
    unsigned int oldCodePage_;   // code page cũ
#else
    termios saved_;              // cấu hình terminal cũ
#endif
};