// ============================================================
// History.h - Ngăn xếp Undo / Redo tự cài đặt bằng DANH SÁCH LIÊN KẾT KÉP
// (Hàng đợi hai đầu - Deque: đẩy/lấy ở cuối, bỏ thao tác cũ nhất ở đầu khi giới hạn k bước)
// ============================================================
#pragma once
#include <cstddef>
#include "EditCommand.h"

struct CmdNode {
    EditCommand* cmd;
    CmdNode* prev;
    CmdNode* next;
    CmdNode(EditCommand* c) : cmd(c), prev(nullptr), next(nullptr) {}
};

// Hàng đợi hai đầu (Deque) tự cài đặt bằng DSLK kép, sở hữu bộ nhớ của các EditCommand*
class CommandDeque {
public:
    CommandDeque() : head_(nullptr), tail_(nullptr), size_(0) {}
    ~CommandDeque() { clear(); }

    CommandDeque(const CommandDeque&) = delete;
    CommandDeque& operator=(const CommandDeque&) = delete;

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    EditCommand* back() const { return tail_ ? tail_->cmd : nullptr; }

    void pushBack(EditCommand* cmd) {
        CmdNode* n = new CmdNode(cmd);
        n->prev = tail_;
        if (tail_) tail_->next = n;
        else head_ = n;
        tail_ = n;
        ++size_;
    }

    // Gỡ nút ở cuối (đỉnh stack) và trả về con trỏ lệnh (không delete cmd)
    EditCommand* popBack() {
        if (!tail_) return nullptr;
        CmdNode* n = tail_;
        EditCommand* c = n->cmd;
        tail_ = n->prev;
        if (tail_) tail_->next = nullptr;
        else head_ = nullptr;
        delete n;
        --size_;
        return c;
    }

    // Gỡ và giải phóng luôn nút cũ nhất ở đầu (dùng khi vượt quá k bước Undo)
    void popFrontAndDelete() {
        if (!head_) return;
        CmdNode* n = head_;
        head_ = n->next;
        if (head_) head_->prev = nullptr;
        else tail_ = nullptr;
        delete n->cmd;
        delete n;
        --size_;
    }

    // Giải phóng toàn bộ các nút và đối tượng lệnh bên trong
    void clear() {
        CmdNode* p = head_;
        while (p) {
            CmdNode* nx = p->next;
            delete p->cmd;
            delete p;
            p = nx;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

private:
    CmdNode* head_;
    CmdNode* tail_;
    size_t size_;
};

class History {
public:
    History() : maxHistory_(0) {} // 0 = không giới hạn số bước

    // Thực hiện lệnh mới, thử gộp vào đỉnh Undo nếu allowMerge == true.
    // History sở hữu cmd (cấp phát bằng new).
    void execute(EditCommand* cmd, Document& doc, Cursor& cur, bool allowMerge = true);
    bool undo(Document& doc, Cursor& cur); // false nếu không còn gì để Undo
    bool redo(Document& doc, Cursor& cur); // false nếu không còn gì để Redo
    void clear();                          // xóa cả hai ngăn xếp (khi mở file mới)

    void setMaxHistory(size_t k);          // k = 0: không giới hạn; k > 0: giữ tối đa k bước Undo
    size_t maxHistory() const { return maxHistory_; }

    size_t undoSize() const { return undo_.size(); }
    size_t redoSize() const { return redo_.size(); }

private:
    CommandDeque undo_; // ngăn xếp Undo (đỉnh ở tail)
    CommandDeque redo_; // ngăn xếp Redo (đỉnh ở tail)
    size_t maxHistory_; // giới hạn số bước Undo (0 = vô hạn)
};