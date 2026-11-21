// ============================================================
// SearchReplace.h - Tìm kiếm, thay thế, thống kê (chỉ ĐỌC văn bản,
// không sửa trực tiếp; việc sửa do ReplaceAllCmd thực hiện để có Undo)
// ============================================================
#pragma once
#include <string>
#include "DynArray.h"
#include "Document.h"
#include "Commands.h" // LineChange

// Một vị trí tìm thấy
struct Match {
    int row; // dòng (từ 0)
    int col; // cột (từ 0)
};

// Kết quả thống kê
struct Stats {
    int lines;            // số dòng
    int words;            // số từ
    int chars;            // số ký tự (không tính ký tự xuống dòng)
    int charsWithNewline; // số ký tự tính cả ký tự xuống dòng giữa các dòng
};

class SearchReplace {
public:
    // Tìm mọi lần xuất hiện của keyword (không chồng lấn). keyword rỗng => danh sách rỗng.
    static DynArray<Match> findAll(const Document& doc, const std::string& keyword);

    // Tính trước các dòng sẽ bị thay đổi khi thay keyword bằng replacement.
    // outCount = tổng số vị trí được thay. Không sửa doc.
    static DynArray<LineChange> computeReplace(const Document& doc, const std::string& keyword,
        const std::string& replacement, int& outCount);

    // Đếm số dòng, số từ, số ký tự
    static Stats computeStats(const Document& doc);
};