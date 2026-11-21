// ============================================================
// Commands.cpp - Cài đặt execute()/undo() cho từng lệnh
// Thư viện: <algorithm> (std::min)
// ============================================================
#include "Commands.h"
#include <algorithm>

// ---------- InsertTextCmd ----------
InsertTextCmd::InsertTextCmd(int row, int col, const std::string& text, bool mergeable)
    : row_(row), col_(col), text_(text), mergeable_(mergeable) {
}

void InsertTextCmd::execute(Document& doc, Cursor& cur) {
    doc.text(row_).insert(col_, text_);
    cur.set(row_, col_ + (int)text_.size());
}
void InsertTextCmd::undo(Document& doc, Cursor& cur) {
    doc.text(row_).erase(col_, text_.size());
    cur.set(row_, col_);
}
bool InsertTextCmd::tryMerge(const EditCommand* next) {
    if (!mergeable_) return false;
    const InsertTextCmd* other = dynamic_cast<const InsertTextCmd*>(next);
    if (!other || !other->mergeable_) return false;
    if (other->row_ == row_ && other->col_ == col_ + (int)text_.size()) {
        text_ += other->text_;
        return true;
    }
    return false;
}

// ---------- DeleteTextCmd ----------
DeleteTextCmd::DeleteTextCmd(int row, int col, int len, int cursorColBefore)
    : row_(row), col_(col), len_(len), cursorColBefore_(cursorColBefore) {
}

void DeleteTextCmd::execute(Document& doc, Cursor& cur) {
    std::string& s = doc.text(row_);
    removed_ = s.substr(col_, len_); // lưu lại để Undo
    s.erase(col_, len_);
    cur.set(row_, col_);
}
void DeleteTextCmd::undo(Document& doc, Cursor& cur) {
    doc.text(row_).insert(col_, removed_);
    cur.set(row_, cursorColBefore_);
}

// ---------- SplitLineCmd ----------
SplitLineCmd::SplitLineCmd(int row, int col) : row_(row), col_(col) {}

void SplitLineCmd::execute(Document& doc, Cursor& cur) {
    doc.splitLine(row_, col_);
    cur.set(row_ + 1, 0);
}
void SplitLineCmd::undo(Document& doc, Cursor& cur) {
    doc.mergeWithNext(row_);
    cur.set(row_, col_);
}

// ---------- MergeLinesCmd ----------
MergeLinesCmd::MergeLinesCmd(int row, int cursorRowBefore, int cursorColBefore)
    : row_(row), beforeRow_(cursorRowBefore), beforeCol_(cursorColBefore), joinCol_(0) {
}

void MergeLinesCmd::execute(Document& doc, Cursor& cur) {
    joinCol_ = (int)doc.text(row_).size(); // ghi nhớ chỗ nối
    doc.mergeWithNext(row_);
    cur.set(row_, joinCol_);
}
void MergeLinesCmd::undo(Document& doc, Cursor& cur) {
    doc.splitLine(row_, joinCol_); // tách lại đúng chỗ đã nối
    cur.set(beforeRow_, beforeCol_);
}

// ---------- DeleteLinesCmd ----------
DeleteLinesCmd::DeleteLinesCmd(int row, int count, int cursorRowBefore, int cursorColBefore)
    : row_(row), count_(count), beforeRow_(cursorRowBefore), beforeCol_(cursorColBefore), allDeleted_(false) {
}

void DeleteLinesCmd::execute(Document& doc, Cursor& cur) {
    texts_.clear();
    allDeleted_ = (count_ >= doc.lineCount());
    for (int i = 0; i < count_; ++i)
        texts_.push_back(doc.removeLine(row_)); // sau mỗi lần xóa, dòng kế dồn lên vị trí row_
    cur.set(std::min(row_, doc.lineCount() - 1), 0);
}
void DeleteLinesCmd::undo(Document& doc, Cursor& cur) {
    if (allDeleted_) {
        doc.text(0) = texts_[0]; // dòng rỗng còn lại chính là dòng đầu
        for (size_t i = 1; i < texts_.size(); ++i) doc.insertLine((int)i, texts_[i]);
    }
    else {
        for (size_t i = 0; i < texts_.size(); ++i) doc.insertLine(row_ + (int)i, texts_[i]);
    }
    cur.set(beforeRow_, beforeCol_);
}

// ---------- PasteLinesCmd ----------
PasteLinesCmd::PasteLinesCmd(int afterRow, const DynArray<std::string>& lines, int cursorRowBefore, int cursorColBefore)
    : afterRow_(afterRow), lines_(lines), beforeRow_(cursorRowBefore), beforeCol_(cursorColBefore) {
}

void PasteLinesCmd::execute(Document& doc, Cursor& cur) {
    for (size_t i = 0; i < lines_.size(); ++i) doc.insertLine(afterRow_ + 1 + (int)i, lines_[i]);
    int last = afterRow_ + (int)lines_.size();
    cur.set(last, (int)doc.text(last).size()); // con trỏ ở cuối đoạn vừa dán
}
void PasteLinesCmd::undo(Document& doc, Cursor& cur) {
    for (size_t i = 0; i < lines_.size(); ++i) doc.removeLine(afterRow_ + 1);
    cur.set(beforeRow_, beforeCol_);
}

// ---------- ReplaceAllCmd ----------
ReplaceAllCmd::ReplaceAllCmd(const DynArray<LineChange>& changes, int cursorRowBefore, int cursorColBefore)
    : changes_(changes), beforeRow_(cursorRowBefore), beforeCol_(cursorColBefore) {
}

void ReplaceAllCmd::execute(Document& doc, Cursor& cur) {
    for (size_t i = 0; i < changes_.size(); ++i) doc.text(changes_[i].row) = changes_[i].newText;
    int c = std::min(cur.col, (int)doc.text(cur.row).size()); // giữ con trỏ trong phạm vi dòng
    cur.set(cur.row, c);
}
void ReplaceAllCmd::undo(Document& doc, Cursor& cur) {
    for (size_t i = 0; i < changes_.size(); ++i) doc.text(changes_[i].row) = changes_[i].oldText;
    cur.set(beforeRow_, beforeCol_);
}