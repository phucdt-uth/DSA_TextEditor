// ============================================================
// EditCommand.h - Lớp cơ sở của mẫu thiết kế COMMAND (nền tảng Undo/Redo)
// Mỗi thao tác sửa văn bản là 1 đối tượng biết tự thực hiện và tự hoàn tác.
// Redo = gọi lại execute().
// ============================================================
#pragma once
#include "Document.h"
#include "Cursor.h"

class EditCommand {
public:
    virtual ~EditCommand() {}
    virtual void execute(Document& doc, Cursor& cur) = 0; // thực hiện (hoặc làm lại) thao tác
    virtual void undo(Document& doc, Cursor& cur) = 0;    // hoàn tác, đưa văn bản + con trỏ về như trước
    virtual bool tryMerge(const EditCommand* next) { (void)next; return false; } // gộp lệnh gõ liền kề
};