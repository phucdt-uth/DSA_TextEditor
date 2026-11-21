// ============================================================
// UI.cpp
// Thư viện: <string>, <cstdio> (snprintf), <cstdlib> (strtol), <algorithm>
// Mã ANSI dùng trong file: \x1b[H = về góc trên trái, \x1b[r;cH = đặt con trỏ,
// \x1b[K = xóa đến cuối dòng, \x1b[7m = đảo màu, \x1b[0m = màu thường,
// \x1b[43;30m = nền vàng chữ đen (tô sáng tìm kiếm), \x1b[?25l / \x1b[?25h = ẩn / hiện con trỏ.
// ============================================================
#include "UI.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>

// Phím Ctrl+<chữ cái>: CTRL_KEY('S') = 19
#define CTRL_KEY(c) ((c) - 'A' + 1)

static const int GUTTER = 7; // độ rộng cột số dòng: "%4d | "

// Cắt chuỗi còn tối đa w ký tự hiển thị (đếm theo UTF-8, không cắt giữa ký tự)
static std::string fit(const std::string& s, int w) {
    if (w <= 0) return "";
    int n = 0;
    size_t i = 0;
    for (; i < s.size(); ++i) {
        if (((unsigned char)s[i] & 0xC0) != 0x80) { // byte đầu của 1 ký tự
            if (n == w) break;
            ++n;
        }
    }
    return s.substr(0, i);
}

// Tô sáng mọi lần xuất hiện của `kw` trong phần chuỗi đã cắt `visible`
static std::string highlightMatches(const std::string& visible, const std::string& kw) {
    if (kw.empty() || visible.empty()) return visible;
    std::string res;
    size_t pos = 0;
    while (pos < visible.size()) {
        size_t found = visible.find(kw, pos);
        if (found == std::string::npos) {
            res += visible.substr(pos);
            break;
        }
        res += visible.substr(pos, found - pos);
        res += "\x1b[43;30m" + visible.substr(found, kw.size()) + "\x1b[0m";
        pos = found + kw.size();
    }
    return res;
}

// Chuỗi mã ANSI đặt con trỏ tại (hàng, cột) màn hình, tính từ 1
static std::string gotoRC(int r, int c) {
    return "\x1b[" + std::to_string(r) + ";" + std::to_string(c) + "H";
}

UI::UI(Editor& editor)
    : ed_(editor), topRow_(0), leftCol_(0), textHeight_(10), running_(true), highlightSearch_(false) {
}

// ------------------------------------------------------------------
// Vẽ màn hình: 1 dòng tiêu đề | vùng văn bản | thanh trạng thái | thanh phím tắt | dòng thông báo
// ------------------------------------------------------------------
void UI::render() {
    int rows, cols;
    term_.getSize(rows, cols);
    if (rows < 8) rows = 8;
    if (cols < 30) cols = 30;
    textHeight_ = rows - 4;

    const Cursor& c = ed_.cursor();
    const Document& d = ed_.doc();
    int textW = cols - GUTTER;

    // Cuộn sao cho con trỏ luôn nằm trong vùng nhìn thấy
    if (c.row < topRow_) topRow_ = c.row;
    if (c.row >= topRow_ + textHeight_) topRow_ = c.row - textHeight_ + 1;
    if (c.col < leftCol_) leftCol_ = c.col;
    if (c.col >= leftCol_ + textW) leftCol_ = c.col - textW + 1;

    std::string out = "\x1b[?25l"; // ẩn con trỏ trong lúc vẽ

    // --- Tiêu đề ---
    std::string name = ed_.fileName().empty() ? "(chưa đặt tên)" : ed_.fileName();
    std::string title = " TRÌNH SOẠN THẢO VĂN BẢN | Tệp: " + name + (ed_.modified() ? " [*]" : "");
    out += gotoRC(1, 1) + "\x1b[7m" + fit(title, cols) + "\x1b[K\x1b[0m";

    // --- Vùng văn bản kèm số dòng ---
    std::string kw = highlightSearch_ ? ed_.lastKeyword() : "";
    for (int i = 0; i < textHeight_; ++i) {
        int row = topRow_ + i;
        out += gotoRC(i + 2, 1);
        if (row < d.lineCount()) {
            char num[32];
            std::snprintf(num, sizeof(num), "%4d | ", row + 1);
            out += num;
            std::string s = d.text(row);
            for (size_t k = 0; k < s.size(); ++k) // ký tự điều khiển => '?', tránh làm hỏng màn hình
                if ((unsigned char)s[k] < 32) s[k] = '?';
            if (leftCol_ < (int)s.size()) {
                std::string vis = fit(s.substr(leftCol_), textW);
                out += highlightMatches(vis, kw);
            }
        }
        else {
            out += " ~";
        }
        out += "\x1b[K";
    }

    // --- Thanh trạng thái: dòng, cột, Undo/Redo ---
    char st[220];
    if (ed_.maxHistory() > 0) {
        std::snprintf(st, sizeof(st), " Dòng: %d/%d Cột: %d | Undo: %d/%d Redo: %d | Clipboard: %d dòng",
            c.row + 1, d.lineCount(), c.col + 1,
            (int)ed_.undoCount(), (int)ed_.maxHistory(), (int)ed_.redoCount(), (int)ed_.clipboardSize());
    }
    else {
        std::snprintf(st, sizeof(st), " Dòng: %d/%d Cột: %d | Undo: %d Redo: %d | Clipboard: %d dòng",
            c.row + 1, d.lineCount(), c.col + 1,
            (int)ed_.undoCount(), (int)ed_.redoCount(), (int)ed_.clipboardSize());
    }
    out += gotoRC(textHeight_ + 2, 1) + "\x1b[7m" + fit(st, cols) + "\x1b[K\x1b[0m";

    // --- Thanh phím tắt ---
    std::string help = "CTRL: ^S Lưu | ^O Mở | ^Z Undo | ^Y Redo | ^K GiớiHạn | ^F Tìm | ^R Thay | ^B Ngoặc | ^C Chép | ^X Cắt | ^P Dán | ^D Xóa | ^T ThốngKê | ^G Giúp | ^Q Thoát";
    out += gotoRC(textHeight_ + 3, 1) + "\x1b[7m" + fit(help, cols) + "\x1b[K\x1b[0m";

    // --- Dòng thông báo ---
    out += gotoRC(rows, 1) + fit(message_, cols - 1) + "\x1b[K";

    // --- Đặt con trỏ thật vào đúng vị trí soạn thảo ---
    out += gotoRC(c.row - topRow_ + 2, GUTTER + (c.col - leftCol_) + 1) + "\x1b[?25h";
    term_.write(out);
}

// ------------------------------------------------------------------
// Ô nhập một dòng ở dòng cuối màn hình. Enter = đồng ý, ESC = hủy.
// ------------------------------------------------------------------
bool UI::prompt(const std::string& label, std::string& out, const std::string& initial) {
    std::string buf = initial;
    while (true) {
        int rows, cols;
        term_.getSize(rows, cols);
        term_.write(gotoRC(rows, 1) + "\x1b[0m" + fit(label + buf, cols - 1) + "\x1b[K\x1b[?25h");
        int k = term_.readKey();
        if (k == K_ENTER) { out = buf; return true; }
        if (k == K_ESC) return false;
        if (k == K_BACKSPACE) { if (!buf.empty()) buf.erase(buf.size() - 1); }
        else if (k >= 32 && k < 127) buf += (char)k;
    }
}

bool UI::confirm(const std::string& question) {
    int rows, cols;
    term_.getSize(rows, cols);
    term_.write(gotoRC(rows, 1) + "\x1b[0m" + fit(question, cols - 1) + "\x1b[K\x1b[?25h");
    int k = term_.readKey();
    return (k == 'y' || k == 'Y');
}

// ------------------------------------------------------------------
// Các lệnh
// ------------------------------------------------------------------
void UI::cmdOpen() {
    if (ed_.modified() && !confirm("Có thay đổi chưa lưu, vẫn mở tệp khác? (y/n)")) return;
    std::string path;
    if (!prompt("Mở tệp: ", path) || path.empty()) { message_ = "Đã hủy mở tệp."; return; }
    std::string err;
    if (ed_.openFile(path, err)) {
        highlightSearch_ = false;
        message_ = "Đã mở: " + path;
    }
    else message_ = err;
}

void UI::cmdSave(bool saveAs) {
    std::string path = ed_.fileName();
    if (saveAs || path.empty()) {
        if (!prompt("Lưu vào tệp: ", path, ed_.fileName()) || path.empty()) { message_ = "Đã hủy lưu."; return; }
    }
    std::string err;
    if (ed_.saveFile(path, err)) message_ = "Đã lưu: " + path;
    else message_ = err;
}

void UI::cmdFind() {
    std::string kw;
    std::string label = ed_.lastKeyword().empty() ? "Tìm: " : "Tìm [Enter = '" + ed_.lastKeyword() + "']: ";
    if (!prompt(label, kw)) { message_ = "Đã hủy tìm kiếm."; return; }
    if (kw.empty()) kw = ed_.lastKeyword();
    if (kw.empty()) return;
    int index = 0;
    int total = ed_.find(kw, index);
    if (total == 0) {
        highlightSearch_ = false;
        message_ = "Không tìm thấy '" + kw + "'.";
    }
    else {
        highlightSearch_ = true;
        message_ = "Kết quả " + std::to_string(index) + "/" + std::to_string(total) +
            " (^F rồi Enter để tìm tiếp, ESC tắt tô sáng).";
    }
}

void UI::cmdReplace() {
    std::string kw, rep;
    std::string label = ed_.lastKeyword().empty() ? "Thay (tìm): " : "Thay (tìm) [Enter = '" + ed_.lastKeyword() + "']: ";
    if (!prompt(label, kw)) { message_ = "Đã hủy."; return; }
    if (kw.empty()) kw = ed_.lastKeyword();
    if (kw.empty()) { message_ = "Từ khóa rỗng."; return; }
    if (!prompt("Thay '" + kw + "' bằng: ", rep)) { message_ = "Đã hủy."; return; }
    int n = ed_.replaceAll(kw, rep);
    highlightSearch_ = false;
    message_ = (n == 0) ? "Không có gì để thay." : "Đã thay " + std::to_string(n) + " vị trí (^Z để hoàn tác).";
}

// Sao chép / cắt: nhập "a" hoặc "a-b" (số dòng tính từ 1); Enter = dòng hiện tại
void UI::cmdCopyCut(bool cut) {
    std::string s;
    if (!prompt(std::string(cut ? "Cắt" : "Chép") + " dòng (a hoặc a-b, Enter = dòng hiện tại): ", s)) {
        message_ = "Đã hủy.";
        return;
    }
    int a = ed_.cursor().row + 1, b = a;
    if (!s.empty()) {
        const char* p = s.c_str();
        char* end = nullptr;
        long x = std::strtol(p, &end, 10);
        bool ok = (end != p);
        long y = x;
        if (ok && *end == '-') {
            p = end + 1;
            y = std::strtol(p, &end, 10);
            ok = (end != p);
        }
        while (ok && *end == ' ') ++end;
        if (!ok || *end != '\0') { message_ = "Sai định dạng. Ví dụ: 3 hoặc 3-5"; return; }
        a = (int)x;
        b = (int)y;
    }
    bool ok = cut ? ed_.cutLines(a, b) : ed_.copyLines(a, b);
    if (!ok) { message_ = "Khoảng dòng không hợp lệ."; return; }
    message_ = std::string(cut ? "Đã cắt " : "Đã chép ") + std::to_string(b - a + 1) + " dòng.";
}

void UI::cmdStats() {
    Stats st = ed_.stats();
    message_ = "Số dòng: " + std::to_string(st.lines) + " | Số từ: " + std::to_string(st.words) +
        " | Số ký tự: " + std::to_string(st.chars) +
        " (tính cả xuống dòng: " + std::to_string(st.charsWithNewline) + ")";
}

void UI::cmdCheckBrackets() {
    BracketResult res = ed_.checkBrackets();
    message_ = res.msg;
    if (!res.ok && res.row >= 0 && res.col >= 0) {
        ed_.moveTo(res.row, res.col); // nhảy con trỏ tới đúng vị trí ngoặc lỗi
    }
}

void UI::cmdSetUndoLimit() {
    std::string s;
    std::string curStr = (ed_.maxHistory() == 0) ? "0 (không giới hạn)" : std::to_string(ed_.maxHistory());
    if (!prompt("Giới hạn bước Undo k (hiện tại: " + curStr + ", nhập 0 = vô hạn): ", s)) {
        message_ = "Đã hủy.";
        return;
    }
    if (s.empty()) return;
    char* end = nullptr;
    long k = std::strtol(s.c_str(), &end, 10);
    while (*end == ' ') ++end;
    if (end == s.c_str() || *end != '\0' || k < 0) {
        message_ = "Giá trị k không hợp lệ (phải là số nguyên >= 0).";
        return;
    }
    ed_.setMaxHistory((size_t)k);
    if (k == 0) message_ = "Đã tắt giới hạn Undo (không giới hạn số bước).";
    else message_ = "Đã đặt giới hạn Undo tối đa " + std::to_string(k) + " bước.";
}

void UI::cmdQuit() {
    if (ed_.modified() && !confirm("Có thay đổi chưa lưu, vẫn thoát? (y/n)")) return;
    running_ = false;
}

void UI::showHelp() {
    int rows, cols;
    term_.getSize(rows, cols);
    const char* lines[] = {
        "TRỢ GIÚP - PHÍM TẮT",
        "",
        "  Mũi tên         Di chuyển con trỏ (lên, xuống, trái, phải)",
        "  Home / End      Về đầu / cuối dòng",
        "  PgUp / PgDn     Nhảy một trang",
        "  Enter           Xuống dòng (tách dòng)",
        "  Backspace/Del   Xóa ký tự (đầu/cuối dòng thì nối dòng)",
        "  Tab             Chèn 4 dấu cách",
        "  ESC             Tắt tô sáng tìm kiếm",
        "",
        "  Ctrl+S          Lưu tệp               Ctrl+W   Lưu thành...",
        "  Ctrl+O          Mở tệp",
        "  Ctrl+Z          Hoàn tác (Undo)       Ctrl+Y   Làm lại (Redo)",
        "  Ctrl+K          Giới hạn số bước Undo (k bước)",
        "  Ctrl+F          Tìm chuỗi             Ctrl+R   Thay thế tất cả",
        "  Ctrl+B          Kiểm tra cặp ngoặc () [] {} cân bằng",
        "  Ctrl+C          Sao chép dòng         Ctrl+X   Cắt dòng",
        "  Ctrl+P          Dán (vào sau dòng hiện tại)",
        "  Ctrl+D          Xóa dòng hiện tại",
        "  Ctrl+T          Thống kê số dòng / từ / ký tự",
        "  Ctrl+Q          Thoát",
        "",
        "  Nhấn phím bất kỳ để quay lại..."
    };
    std::string out = "\x1b[?25l\x1b[0m\x1b[2J";
    int n = (int)(sizeof(lines) / sizeof(lines[0]));
    for (int i = 0; i < n && i < rows; ++i) out += gotoRC(i + 1, 1) + fit(lines[i], cols) + "\x1b[K";
    term_.write(out);
    term_.readKey();
    term_.write("\x1b[2J");
}

// ------------------------------------------------------------------
// Xử lý phím
// ------------------------------------------------------------------
void UI::handleKey(int key) {
    message_.clear(); // thông báo cũ biến mất khi có phím mới
    switch (key) {
    case K_UP:        ed_.moveUp(); break;
    case K_DOWN:      ed_.moveDown(); break;
    case K_LEFT:      ed_.moveLeft(); break;
    case K_RIGHT:     ed_.moveRight(); break;
    case K_HOME:      ed_.moveHome(); break;
    case K_END:       ed_.moveEnd(); break;
    case K_PGUP:      ed_.movePage(-textHeight_); break;
    case K_PGDN:      ed_.movePage(textHeight_); break;
    case K_ENTER:     ed_.enter(); break;
    case K_BACKSPACE: ed_.backspace(); break;
    case K_DELETE:    ed_.del(); break;
    case K_TAB:       ed_.insertTab(); break;
    case K_ESC:       highlightSearch_ = false; break;
    case K_NONE:      break;
    default:
        if (key == CTRL_KEY('S'))      cmdSave(false);
        else if (key == CTRL_KEY('W')) cmdSave(true);
        else if (key == CTRL_KEY('O')) cmdOpen();
        else if (key == CTRL_KEY('Z')) { if (!ed_.undo()) message_ = "Không còn gì để Undo."; }
        else if (key == CTRL_KEY('Y')) { if (!ed_.redo()) message_ = "Không còn gì để Redo."; }
        else if (key == CTRL_KEY('K')) cmdSetUndoLimit();
        else if (key == CTRL_KEY('F')) cmdFind();
        else if (key == CTRL_KEY('R')) cmdReplace();
        else if (key == CTRL_KEY('B')) cmdCheckBrackets();
        else if (key == CTRL_KEY('C')) cmdCopyCut(false);
        else if (key == CTRL_KEY('X')) cmdCopyCut(true);
        else if (key == CTRL_KEY('P')) { if (!ed_.paste()) message_ = "Clipboard đang rỗng."; }
        else if (key == CTRL_KEY('D')) ed_.deleteLine();
        else if (key == CTRL_KEY('T')) cmdStats();
        else if (key == CTRL_KEY('G')) showHelp();
        else if (key == CTRL_KEY('Q')) cmdQuit();
        else if (key >= 32 && key < 127) ed_.typeChar((char)key); // ký tự in được
        break;
    }
}

void UI::run() {
    term_.write("\x1b[0m\x1b[2J");
    while (running_) {
        render();
        handleKey(term_.readKey());
    }
    term_.write("\x1b[0m\x1b[2J\x1b[H\x1b[?25h"); // dọn màn hình khi thoát
}