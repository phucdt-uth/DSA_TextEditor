// ============================================================
// FileManager.h - Mở / lưu tệp văn bản
// ============================================================
#pragma once
#include <string>
#include "DynArray.h"
#include "Document.h"

class FileManager {
public:
    // Đọc file thành danh sách dòng. Trả về false và điền err nếu lỗi.
    static bool load(const std::string& path, DynArray<std::string>& lines, std::string& err);
    // Ghi toàn bộ Document ra file (mỗi dòng cách nhau '\n').
    static bool save(const std::string& path, const Document& doc, std::string& err);
};