// ============================================================
// SearchReplace.cpp
// Thư viện: <cctype> (isspace)
// ============================================================
#include "SearchReplace.h"
#include <cctype>

DynArray<Match> SearchReplace::findAll(const Document& doc, const std::string& keyword) {
    DynArray<Match> result;
    if (keyword.empty()) return result;
    for (int r = 0; r < doc.lineCount(); ++r) {
        const std::string& s = doc.text(r);
        size_t pos = 0;
        while ((pos = s.find(keyword, pos)) != std::string::npos) {
            Match m; m.row = r; m.col = (int)pos;
            result.push_back(m);
            pos += keyword.size(); // nhảy qua phần vừa tìm thấy
        }
    }
    return result;
}

DynArray<LineChange> SearchReplace::computeReplace(const Document& doc, const std::string& keyword,
    const std::string& replacement, int& outCount) {
    DynArray<LineChange> changes;
    outCount = 0;
    if (keyword.empty()) return changes;
    for (int r = 0; r < doc.lineCount(); ++r) {
        std::string s = doc.text(r); // bản sao để sửa
        size_t pos = 0;
        bool changed = false;
        while ((pos = s.find(keyword, pos)) != std::string::npos) {
            s.replace(pos, keyword.size(), replacement);
            pos += replacement.size(); // QUAN TRỌNG: nhảy qua chuỗi mới, tránh lặp vô hạn
            ++outCount;
            changed = true;
        }
        if (changed) {
            LineChange ch; ch.row = r; ch.oldText = doc.text(r); ch.newText = s;
            changes.push_back(ch);
        }
    }
    return changes;
}

Stats SearchReplace::computeStats(const Document& doc) {
    Stats st;
    st.lines = doc.lineCount();
    st.words = 0;
    st.chars = 0;
    for (int r = 0; r < doc.lineCount(); ++r) {
        const std::string& s = doc.text(r);
        st.chars += (int)s.size();
        bool inWord = false; // đếm số lần chuyển từ "khoảng trắng" sang "chữ"
        for (size_t i = 0; i < s.size(); ++i) {
            if (std::isspace((unsigned char)s[i])) inWord = false;
            else if (!inWord) { inWord = true; ++st.words; }
        }
    }
    st.charsWithNewline = st.chars + (st.lines - 1);
    return st;
}