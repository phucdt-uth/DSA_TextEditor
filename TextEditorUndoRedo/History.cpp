// ============================================================
// History.cpp - Cài đặt Undo/Redo trên DSLK kép (CommandDeque)
// ============================================================
#include "History.h"

void History::clear() {
    undo_.clear();
    redo_.clear();
}

void History::setMaxHistory(size_t k) {
    maxHistory_ = k;
    if (maxHistory_ > 0) {
        while (undo_.size() > maxHistory_) {
            undo_.popFrontAndDelete();
        }
    }
}

void History::execute(EditCommand* cmd, Document& doc, Cursor& cur, bool allowMerge) {
    cmd->execute(doc, cur);
    redo_.clear(); // thao tác mới sau Undo => mất Redo

    // Thử gộp nếu là thao tác gõ chữ liền kề trên cùng dòng
    if (allowMerge && !undo_.empty() && undo_.back()->tryMerge(cmd)) {
        delete cmd; // đã gộp nội dung vào lệnh trên đỉnh, giải phóng lệnh tạm
        return;
    }

    undo_.pushBack(cmd);
    if (maxHistory_ > 0 && undo_.size() > maxHistory_) {
        undo_.popFrontAndDelete(); // bỏ thao tác cũ nhất ở đầu hàng đợi hai đầu
    }
}

bool History::undo(Document& doc, Cursor& cur) {
    if (undo_.empty()) return false;
    EditCommand* cmd = undo_.popBack();
    cmd->undo(doc, cur);
    redo_.pushBack(cmd);
    return true;
}

bool History::redo(Document& doc, Cursor& cur) {
    if (redo_.empty()) return false;
    EditCommand* cmd = redo_.popBack();
    cmd->execute(doc, cur);
    undo_.pushBack(cmd);
    if (maxHistory_ > 0 && undo_.size() > maxHistory_) {
        undo_.popFrontAndDelete();
    }
    return true;
}