// ============================================================
// Document.cpp - Cài đặt danh sách liên kết đôi các dòng
// Thư viện: <cstdlib> (std::abs)
// ============================================================
#include "Document.h"
#include <cstdlib>

Document::Document() : head_(nullptr), tail_(nullptr), count_(0), cacheNode_(nullptr), cacheRow_(0) {
    appendLine(""); // văn bản mới luôn có 1 dòng rỗng
}

Document::~Document() { freeAll(); }

// Giải phóng tất cả nút để không rò rỉ bộ nhớ
void Document::freeAll() {
    Line* p = head_;
    while (p) {
        Line* nx = p->next;
        delete p;
        p = nx;
    }
    head_ = tail_ = nullptr;
    count_ = 0;
    resetCache();
}

void Document::clear() {
    freeAll();
    appendLine("");
}

void Document::setLines(const DynArray<std::string>& lines) {
    freeAll();
    for (size_t i = 0; i < lines.size(); ++i) appendLine(lines[i]);
    if (count_ == 0) appendLine("");
}

// Tìm nút ở chỉ số row. Xuất phát từ đầu, cuối hoặc nút cache - nơi nào gần nhất.
Line* Document::nodeAt(int row) const {
    if (row < 0 || row >= count_) return nullptr;
    Line* p = head_; int pos = 0; int best = row; // khoảng cách từ head
    int dTail = count_ - 1 - row;                  // khoảng cách từ tail
    if (dTail < best) { p = tail_; pos = count_ - 1; best = dTail; }
    if (cacheNode_) {
        int dCache = std::abs(row - cacheRow_);
        if (dCache < best) { p = cacheNode_; pos = cacheRow_; best = dCache; }
    }
    while (pos < row) { p = p->next; ++pos; } // đi xuống (next)
    while (pos > row) { p = p->prev; --pos; } // đi lên (prev)
    cacheNode_ = p;
    cacheRow_ = row;
    return p;
}

std::string& Document::text(int row) { return nodeAt(row)->text; }
const std::string& Document::text(int row) const { return nodeAt(row)->text; }

// Chèn nút mới sao cho nó nằm ở chỉ số row
void Document::insertLine(int row, const std::string& s) {
    Line* n = new Line(s);
    if (row >= count_) { // chèn vào cuối
        n->prev = tail_;
        if (tail_) tail_->next = n; else head_ = n;
        tail_ = n;
    }
    else { // chèn trước nút đang ở vị trí row
        Line* at = nodeAt(row);
        n->next = at;
        n->prev = at->prev;
        if (at->prev) at->prev->next = n; else head_ = n;
        at->prev = n;
    }
    ++count_;
    resetCache();
}

void Document::appendLine(const std::string& s) { insertLine(count_, s); }

// Gỡ nút khỏi danh sách
std::string Document::removeLine(int row) {
    Line* n = nodeAt(row);
    std::string t = n->text;
    if (count_ == 1) { // dòng duy nhất: chỉ xóa nội dung
        n->text.clear();
        return t;
    }
    if (n->prev) n->prev->next = n->next; else head_ = n->next;
    if (n->next) n->next->prev = n->prev; else tail_ = n->prev;
    delete n;
    --count_;
    resetCache();
    return t;
}

// Enter: phần sau con trỏ thành nút mới chèn ngay sau nút hiện tại
void Document::splitLine(int row, int col) {
    std::string& s = nodeAt(row)->text;
    std::string rest = s.substr(col);
    s.erase(col);
    insertLine(row + 1, rest);
}

// Backspace đầu dòng / Delete cuối dòng: nối dòng sau vào dòng này
void Document::mergeWithNext(int row) {
    Line* n = nodeAt(row);
    if (!n || !n->next) return;
    n->text += n->next->text;
    removeLine(row + 1);
}