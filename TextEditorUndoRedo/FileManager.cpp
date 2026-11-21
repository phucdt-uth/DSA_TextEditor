// ============================================================
// FileManager.cpp
// Thư viện: <fstream> (ifstream/ofstream)
// ============================================================
#include "FileManager.h"
#include <fstream>

bool FileManager::load(const std::string& path, DynArray<std::string>& lines, std::string& err) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.is_open()) { err = "Không mở được tệp: " + path; return false; }
    lines.clear();
    std::string line;
    bool first = true;
    while (std::getline(in, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1); // CRLF -> LF
        if (first && line.size() >= 3 && (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF)
            line.erase(0, 3); // bỏ BOM UTF-8
        first = false;
        std::string out; // đổi Tab thành 4 dấu cách để hiển thị đúng cột
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '\t') out += "    "; else out += line[i];
        }
        lines.push_back(out);
    }
    return true; // file rỗng => lines rỗng => Document tự tạo 1 dòng rỗng
}

bool FileManager::save(const std::string& path, const Document& doc, std::string& err) {
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open()) { err = "Không ghi được tệp: " + path; return false; }
    for (int r = 0; r < doc.lineCount(); ++r) {
        out << doc.text(r);
        if (r + 1 < doc.lineCount()) out << '\n'; // không thêm '\n' thừa ở dòng cuối
    }
    out.flush();
    if (!out.good()) { err = "Lỗi khi ghi tệp: " + path; return false; }
    return true;
}