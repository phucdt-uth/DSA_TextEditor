// ============================================================
// Document.h - Văn bản = DANH SÁCH LIÊN KẾT ĐÔI, mỗi nút là một dòng
// ============================================================
#pragma once
#include <string>
#include "DynArray.h"

// Nút của danh sách liên kết đôi: chứa nội dung 1 dòng
struct Line {
    std::string text; // nội dung dòng (không chứa '\n')
    Line* prev;       // nút dòng phía trước (nullptr nếu là dòng đầu)
    Line* next;       // nút dòng phía sau (nullptr nếu là dòng cuối)
    Line(const std::string& t = "") : text(t), prev(nullptr), next(nullptr) {}
};

// Quản lý toàn bộ văn bản. Luôn có ít nhất 1 dòng (văn bản rỗng = 1 dòng rỗng).
// Các hàm nhận "row" là chỉ số dòng bắt đầu từ 0.
class Document {
public:
    Document();
    ~Document();
    Document(const Document&) = delete; // cấm sao chép (tránh lỗi con trỏ)
    Document& operator=(const Document&) = delete;

    int lineCount() const { return count_; } // số dòng hiện có

    void clear();                                        // xóa hết, còn 1 dòng rỗng
    void setLines(const DynArray<std::string>& lines);   // thay toàn bộ nội dung (dùng khi mở file)

    std::string& text(int row);                          // tham chiếu tới nội dung dòng row
    const std::string& text(int row) const;

    void insertLine(int row, const std::string& s);      // chèn dòng mới để nó có chỉ số = row (0..lineCount)
    void appendLine(const std::string& s);               // thêm dòng vào cuối
    std::string removeLine(int row);                     // gỡ nút dòng row, trả về nội dung. Nếu chỉ còn 1 dòng: chỉ xóa nội dung
    void splitLine(int row, int col);                    // tách dòng row tại cột col (phím Enter)
    void mergeWithNext(int row);                         // nối dòng row+1 vào cuối dòng row

private:
    Line* head_;              // nút đầu danh sách
    Line* tail_;              // nút cuối danh sách
    int count_;               // số nút (số dòng)
    mutable Line* cacheNode_; // nút truy cập gần nhất (để duyệt tuần tự nhanh O(1))
    mutable int cacheRow_;    // chỉ số dòng của cacheNode_

    Line* nodeAt(int row) const; // tìm nút theo chỉ số: duyệt từ head / tail / cache, chọn đường ngắn nhất
    void resetCache() const { cacheNode_ = nullptr; cacheRow_ = 0; }
    void freeAll();              // giải phóng mọi nút
};