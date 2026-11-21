// ============================================================
// Editor.h - LÕI soạn thảo: gom Document + Cursor + History + Clipboard.
// Không chứa code giao diện; UI chỉ gọi các hàm ở đây.
// ============================================================
#pragma once
#include <string>
#include "DynArray.h"
#include "Document.h"
#include "Cursor.h"
#include "History.h"
#include "SearchReplace.h"

// Kết quả kiểm tra cặp ngoặc () [] {} cân bằng
struct BracketResult {
    bool ok;          // true nếu toàn bộ ngoặc cân bằng
    int row;          // dòng xảy ra lỗi (từ 0, -1 nếu ok)
    int col;          // cột xảy ra lỗi (từ 0, -1 nếu ok)
    std::string msg;  // mô tả chi tiết lỗi
};

class Editor {
public:
    Editor();

    // ----- Truy cập (chỉ đọc) -----
    const Document& doc() const { return doc_; }
    const Cursor& cursor() const { return cur_; }
    const std::string& fileName() const { return fileName_; }
    void setFileName(const std::string& n) { fileName_ = n; }
    bool modified() const { return modified_; }
    const std::string& lastKeyword() const { return lastKeyword_; }
    size_t undoCount() const { return history_.undoSize(); }
    size_t redoCount() const { return history_.redoSize(); }
    size_t maxHistory() const { return history_.maxHistory(); }
    size_t clipboardSize() const { return clipboard_.size(); }

    // ----- Di chuyển con trỏ -----
    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();
    void moveHome();              // về đầu dòng
    void moveEnd();               // về cuối dòng
    void movePage(int deltaRows); // nhảy nhiều dòng (PageUp/PageDown)
    void moveTo(int row, int col);// nhảy tới tọa độ (row, col)

    // ----- Sửa văn bản (đều đi qua History => Undo được) -----
    void typeChar(char ch);       // chèn ký tự tại con trỏ
    void insertTab();             // chèn 4 dấu cách
    void enter();                 // tách dòng
    void backspace();             // xóa ký tự trước con trỏ / nối dòng
    void del();                   // xóa ký tự tại con trỏ / nối dòng sau
    void deleteLine();            // xóa dòng hiện tại

    // ----- Sao chép / cắt / dán theo dòng (số dòng tính từ 1) -----
    bool copyLines(int from, int to);
    bool cutLines(int from, int to);
    bool paste();                 // dán clipboard vào sau dòng hiện tại

    // ----- Undo / Redo & Giới hạn k bước -----
    bool undo();
    bool redo();
    void setMaxHistory(size_t k);

    // ----- Tìm / thay / thống kê / kiểm tra ngoặc -----
    int find(const std::string& keyword, int& index);
    int replaceAll(const std::string& keyword, const std::string& replacement);
    Stats stats() const;
    BracketResult checkBrackets() const;

    // ----- Tệp -----
    bool openFile(const std::string& path, std::string& err);
    bool saveFile(const std::string& path, std::string& err);

private:
    Document doc_;                       // văn bản (danh sách liên kết đôi)
    Cursor cur_;                         // con trỏ
    History history_;                    // Undo/Redo (DSLK kép)
    DynArray<std::string> clipboard_;    // bộ nhớ đệm các dòng đã chép/cắt
    std::string fileName_;               // tên tệp hiện tại ("" nếu chưa đặt)
    bool modified_;                      // đã sửa kể từ lần lưu cuối?
    int savedUndoCount_;                 // số bước Undo tại lần mở/lưu cuối (-1 nếu mất mốc)
    std::string lastKeyword_;            // từ khóa tìm kiếm gần nhất

    void run(EditCommand* cmd, bool allowMerge = true);
    int lineLen(int row) const { return (int)doc_.text(row).size(); }
};