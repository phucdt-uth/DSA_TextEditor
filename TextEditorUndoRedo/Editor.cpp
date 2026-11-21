// ============================================================
// Editor.cpp
// Thư viện: <algorithm> (std::min, std::max)
// ============================================================
#include "Editor.h"
#include "Commands.h"
#include "FileManager.h"
#include <algorithm>

Editor::Editor() : modified_(false), savedUndoCount_(0) {}

void Editor::run(EditCommand* cmd, bool allowMerge) {
    if (savedUndoCount_ != -1 && (int)history_.undoSize() < savedUndoCount_) {
        savedUndoCount_ = -1;
    }
    size_t beforeSize = history_.undoSize();
    history_.execute(cmd, doc_, cur_, allowMerge);

    if (savedUndoCount_ > 0 && history_.maxHistory() > 0 &&
        beforeSize == history_.maxHistory() && history_.undoSize() == beforeSize) {
        --savedUndoCount_;
        if (savedUndoCount_ < 0) savedUndoCount_ = -1;
    }

    modified_ = ((int)history_.undoSize() != savedUndoCount_);
}

// ---------------- Di chuyển ----------------
void Editor::moveLeft() {
    if (cur_.col > 0) cur_.col--;
    else if (cur_.row > 0) { cur_.row--; cur_.col = lineLen(cur_.row); } // nhảy lên cuối dòng trên
    cur_.wantedCol = cur_.col;
}
void Editor::moveRight() {
    if (cur_.col < lineLen(cur_.row)) cur_.col++;
    else if (cur_.row + 1 < doc_.lineCount()) { cur_.row++; cur_.col = 0; } // xuống đầu dòng dưới
    cur_.wantedCol = cur_.col;
}
void Editor::moveUp() {
    if (cur_.row > 0) {
        cur_.row--;
        cur_.col = std::min(cur_.wantedCol, lineLen(cur_.row)); // giữ cột mong muốn
    }
}
void Editor::moveDown() {
    if (cur_.row + 1 < doc_.lineCount()) {
        cur_.row++;
        cur_.col = std::min(cur_.wantedCol, lineLen(cur_.row));
    }
}
void Editor::moveHome() { cur_.col = 0; cur_.wantedCol = 0; }
void Editor::moveEnd() { cur_.col = lineLen(cur_.row); cur_.wantedCol = cur_.col; }
void Editor::movePage(int d) {
    int r = std::max(0, std::min(doc_.lineCount() - 1, cur_.row + d));
    cur_.row = r;
    cur_.col = std::min(cur_.wantedCol, lineLen(r));
}
void Editor::moveTo(int row, int col) {
    int r = std::max(0, std::min(doc_.lineCount() - 1, row));
    int c = std::max(0, std::min(lineLen(r), col));
    cur_.set(r, c);
}

// ---------------- Sửa văn bản ----------------
void Editor::typeChar(char ch) {
    bool atSavePoint = (savedUndoCount_ != -1 && (int)history_.undoSize() == savedUndoCount_);
    bool canMerge = (ch != ' ') && !atSavePoint;
    run(new InsertTextCmd(cur_.row, cur_.col, std::string(1, ch), canMerge), canMerge);
}
void Editor::insertTab() { run(new InsertTextCmd(cur_.row, cur_.col, "    ", false), false); }
void Editor::enter() { run(new SplitLineCmd(cur_.row, cur_.col), false); }

void Editor::backspace() {
    if (cur_.col > 0)
        run(new DeleteTextCmd(cur_.row, cur_.col - 1, 1, cur_.col), false); // xóa ký tự bên trái
    else if (cur_.row > 0)
        run(new MergeLinesCmd(cur_.row - 1, cur_.row, cur_.col), false);   // nối dòng này vào dòng trên
}
void Editor::del() {
    if (cur_.col < lineLen(cur_.row))
        run(new DeleteTextCmd(cur_.row, cur_.col, 1, cur_.col), false);     // xóa ký tự tại con trỏ
    else if (cur_.row + 1 < doc_.lineCount())
        run(new MergeLinesCmd(cur_.row, cur_.row, cur_.col), false);       // nối dòng dưới lên
}
void Editor::deleteLine() { run(new DeleteLinesCmd(cur_.row, 1, cur_.row, cur_.col), false); }

// ---------------- Clipboard ----------------
bool Editor::copyLines(int from, int to) {
    if (from < 1 || to < from || to > doc_.lineCount()) return false;
    clipboard_.clear();
    for (int r = from - 1; r <= to - 1; ++r) clipboard_.push_back(doc_.text(r));
    return true;
}
bool Editor::cutLines(int from, int to) {
    if (!copyLines(from, to)) return false;
    run(new DeleteLinesCmd(from - 1, to - from + 1, cur_.row, cur_.col), false); // cắt = chép + xóa (Undo được)
    return true;
}
bool Editor::paste() {
    if (clipboard_.empty()) return false;
    run(new PasteLinesCmd(cur_.row, clipboard_, cur_.row, cur_.col), false);
    return true;
}

// ---------------- Undo / Redo ----------------
bool Editor::undo() {
    bool ok = history_.undo(doc_, cur_);
    if (ok) modified_ = ((int)history_.undoSize() != savedUndoCount_);
    return ok;
}
bool Editor::redo() {
    bool ok = history_.redo(doc_, cur_);
    if (ok) modified_ = ((int)history_.undoSize() != savedUndoCount_);
    return ok;
}
void Editor::setMaxHistory(size_t k) {
    size_t before = history_.undoSize();
    history_.setMaxHistory(k);
    if (savedUndoCount_ != -1 && history_.undoSize() < before) {
        size_t dropped = before - history_.undoSize();
        savedUndoCount_ = ((int)dropped > savedUndoCount_) ? -1 : (savedUndoCount_ - (int)dropped);
    }
    modified_ = ((int)history_.undoSize() != savedUndoCount_);
}

// ---------------- Tìm / thay / thống kê / kiểm tra ngoặc ----------------
int Editor::find(const std::string& keyword, int& index) {
    lastKeyword_ = keyword;
    index = 0;
    DynArray<Match> ms = SearchReplace::findAll(doc_, keyword);
    if (ms.empty()) return 0;
    size_t pick = 0;
    for (size_t i = 0; i < ms.size(); ++i) {
        if (ms[i].row > cur_.row || (ms[i].row == cur_.row && ms[i].col > cur_.col)) { pick = i; break; }
    }
    cur_.set(ms[pick].row, ms[pick].col);
    index = (int)pick + 1;
    return (int)ms.size();
}

int Editor::replaceAll(const std::string& keyword, const std::string& replacement) {
    int count = 0;
    DynArray<LineChange> ch = SearchReplace::computeReplace(doc_, keyword, replacement, count);
    if (!ch.empty()) run(new ReplaceAllCmd(ch, cur_.row, cur_.col), false);
    lastKeyword_ = keyword;
    return count;
}

Stats Editor::stats() const { return SearchReplace::computeStats(doc_); }

struct BracketNode {
    char ch;
    int row;
    int col;
    BracketNode* next;
    BracketNode(char c, int r, int cl, BracketNode* nx) : ch(c), row(r), col(cl), next(nx) {}
};

static void freeBrackets(BracketNode* top) {
    while (top) {
        BracketNode* tmp = top;
        top = top->next;
        delete tmp;
    }
}

BracketResult Editor::checkBrackets() const {
    BracketNode* top = nullptr;

    for (int r = 0; r < doc_.lineCount(); ++r) {
        const std::string& s = doc_.text(r);
        for (int c = 0; c < (int)s.size(); ++c) {
            char ch = s[c];
            if (ch == '(' || ch == '[' || ch == '{') {
                top = new BracketNode(ch, r, c, top);
            }
            else if (ch == ')' || ch == ']' || ch == '}') {
                if (!top) {
                    BracketResult res;
                    res.ok = false; res.row = r; res.col = c;
                    res.msg = "Thừa ngoặc đóng '" + std::string(1, ch) + "' tại dòng " +
                        std::to_string(r + 1) + ", cột " + std::to_string(c + 1);
                    return res;
                }
                char expected = (top->ch == '(') ? ')' : (top->ch == '[') ? ']' : '}';
                if (ch != expected) {
                    int openR = top->row, openC = top->col;
                    char openCh = top->ch;
                    freeBrackets(top);
                    BracketResult res;
                    res.ok = false; res.row = r; res.col = c;
                    res.msg = "Ngoặc '" + std::string(1, ch) + "' tại dòng " + std::to_string(r + 1) +
                        ", cột " + std::to_string(c + 1) + " không khớp '" + std::string(1, openCh) +
                        "' ở dòng " + std::to_string(openR + 1) + ", cột " + std::to_string(openC + 1);
                    return res;
                }
                BracketNode* matched = top;
                top = top->next;
                delete matched;
            }
        }
    }

    if (top) {
        int openR = top->row, openC = top->col;
        char openCh = top->ch;
        freeBrackets(top);
        BracketResult res;
        res.ok = false; res.row = openR; res.col = openC;
        res.msg = "Thiếu ngoặc đóng cho '" + std::string(1, openCh) + "' tại dòng " +
            std::to_string(openR + 1) + ", cột " + std::to_string(openC + 1);
        return res;
    }

    BracketResult okRes;
    okRes.ok = true; okRes.row = -1; okRes.col = -1;
    okRes.msg = "Tất cả cặp ngoặc () [] {} đều cân bằng!";
    return okRes;
}

// ---------------- Tệp ----------------
bool Editor::openFile(const std::string& path, std::string& err) {
    DynArray<std::string> lines;
    if (!FileManager::load(path, lines, err)) return false;
    doc_.setLines(lines);
    cur_ = Cursor();      // về dòng 1, cột 0
    history_.clear();     // mở file mới => xóa lịch sử Undo/Redo
    fileName_ = path;
    savedUndoCount_ = 0;
    modified_ = false;
    return true;
}

bool Editor::saveFile(const std::string& path, std::string& err) {
    if (!FileManager::save(path, doc_, err)) return false;
    fileName_ = path;
    savedUndoCount_ = (int)history_.undoSize();
    modified_ = false;
    return true;
}