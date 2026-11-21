// ============================================================
// Commands.h - Các lệnh cụ thể (mỗi lệnh lưu đủ dữ liệu để đảo ngược)
// ============================================================
#pragma once
#include "EditCommand.h"
#include "DynArray.h"
#include <string>

// Chèn chuỗi vào trong 1 dòng (gõ ký tự, Tab)
class InsertTextCmd : public EditCommand {
public:
    InsertTextCmd(int row, int col, const std::string& text, bool mergeable = false);
    void execute(Document& doc, Cursor& cur);
    void undo(Document& doc, Cursor& cur);
    bool tryMerge(const EditCommand* next);
private:
    int row_, col_;    // vị trí chèn
    std::string text_; // chuỗi được chèn
    bool mergeable_;   // cho phép gộp ký tự gõ liền kề
};

// Xóa len ký tự trong 1 dòng bắt đầu từ col (Backspace / Delete)
class DeleteTextCmd : public EditCommand {
public:
    DeleteTextCmd(int row, int col, int len, int cursorColBefore);
    void execute(Document& doc, Cursor& cur);
    void undo(Document& doc, Cursor& cur);
private:
    int row_, col_, len_;
    int cursorColBefore_; // cột con trỏ trước khi xóa (để Undo trả về đúng chỗ)
    std::string removed_; // chuỗi đã bị xóa (để chèn lại khi Undo)
};

// Enter: tách dòng row tại cột col
class SplitLineCmd : public EditCommand {
public:
    SplitLineCmd(int row, int col);
    void execute(Document& doc, Cursor& cur);
    void undo(Document& doc, Cursor& cur);
private:
    int row_, col_;
};

// Backspace đầu dòng / Delete cuối dòng: nối dòng row+1 vào dòng row
class MergeLinesCmd : public EditCommand {
public:
    MergeLinesCmd(int row, int cursorRowBefore, int cursorColBefore);
    void execute(Document& doc, Cursor& cur);
    void undo(Document& doc, Cursor& cur);
private:
    int row_;
    int beforeRow_, beforeCol_; // vị trí con trỏ trước khi nối
    int joinCol_;               // độ dài dòng row trước khi nối = chỗ để tách lại
};

// Xóa / Cắt `count` dòng liên tiếp từ dòng row
class DeleteLinesCmd : public EditCommand {
public:
    DeleteLinesCmd(int row, int count, int cursorRowBefore, int cursorColBefore);
    void execute(Document& doc, Cursor& cur);
    void undo(Document& doc, Cursor& cur);
private:
    int row_, count_;
    int beforeRow_, beforeCol_;
    bool allDeleted_;                // true nếu xóa toàn bộ văn bản (còn lại 1 dòng rỗng)
    DynArray<std::string> texts_;    // nội dung các dòng đã xóa
};

// Dán các dòng vào sau dòng afterRow
class PasteLinesCmd : public EditCommand {
public:
    PasteLinesCmd(int afterRow, const DynArray<std::string>& lines, int cursorRowBefore, int cursorColBefore);
    void execute(Document& doc, Cursor& cur);
    void undo(Document& doc, Cursor& cur);
private:
    int afterRow_;
    DynArray<std::string> lines_;
    int beforeRow_, beforeCol_;
};

// Thay đổi của 1 dòng khi "Thay thế tất cả"
struct LineChange {
    int row;
    std::string oldText;
    std::string newText;
};

// Thay thế tất cả = 1 lệnh duy nhất => 1 lần Undo hoàn tác cả khối
class ReplaceAllCmd : public EditCommand {
public:
    ReplaceAllCmd(const DynArray<LineChange>& changes, int cursorRowBefore, int cursorColBefore);
    void execute(Document& doc, Cursor& cur);
    void undo(Document& doc, Cursor& cur);
private:
    DynArray<LineChange> changes_;
    int beforeRow_, beforeCol_;
};