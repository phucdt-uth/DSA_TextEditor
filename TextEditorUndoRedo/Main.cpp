// ============================================================
// main.cpp - Điểm vào chương trình
// Cách chạy:  editor            (văn bản trống)
//             editor file.txt   (mở sẵn tệp; nếu chưa có thì tạo khi lưu)
// ============================================================
#include <iostream>
#include "Editor.h"
#include "UI.h"

int main(int argc, char** argv) {
    Editor editor;
    std::string startMsg = "Nhấn Ctrl+G để xem trợ giúp.";

    if (argc > 1) {
        std::string err;
        if (!editor.openFile(argv[1], err)) {
            editor.setFileName(argv[1]);            // tệp chưa tồn tại: sẽ được tạo khi lưu
            startMsg = "Tệp mới: " + std::string(argv[1]);
        }
    }

    UI ui(editor);
    ui.setMessage(startMsg);
    ui.run();
    return 0;
}