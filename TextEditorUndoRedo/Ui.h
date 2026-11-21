// ============================================================
// UI.h - Giao diện console: vẽ màn hình, đọc phím, hộp nhập liệu
// ============================================================
#pragma once
#include <string>
#include "Editor.h"
#include "Terminal.h"

class UI {
public:
    explicit UI(Editor& editor);
    void setMessage(const std::string& m) { message_ = m; } // thông báo hiện ở dòng cuối
    void run(); // vòng lặp chính: vẽ -> đọc phím -> xử lý

private:
    Editor& ed_;          // lõi soạn thảo
    Terminal term_;       // console (tạo ra là bật raw mode)
    int topRow_;          // dòng đầu tiên đang hiển thị (cuộn dọc)
    int leftCol_;         // cột đầu tiên đang hiển thị (cuộn ngang)
    int textHeight_;      // số dòng vùng soạn thảo (cập nhật mỗi lần vẽ)
    std::string message_; // thông báo cho người dùng
    bool running_;        // false => thoát vòng lặp
    bool highlightSearch_;// bật tô sáng từ khóa vừa tìm kiếm

    void render();             // vẽ toàn bộ màn hình
    void handleKey(int key);   // xử lý 1 phím
    bool prompt(const std::string& label, std::string& out, const std::string& initial = ""); // ô nhập 1 dòng (ESC = hủy)
    bool confirm(const std::string& question); // hỏi có/không (y/n)
    void showHelp();           // màn hình trợ giúp

    // Các lệnh tương ứng phím tắt
    void cmdOpen();
    void cmdSave(bool saveAs);
    void cmdFind();
    void cmdReplace();
    void cmdCopyCut(bool cut);
    void cmdStats();
    void cmdCheckBrackets();
    void cmdSetUndoLimit();
    void cmdQuit();
};