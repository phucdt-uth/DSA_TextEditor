// ============================================================
// Cursor.h - Cấu trúc con trỏ soạn thảo
// ============================================================
#pragma once

// Con trỏ soạn thảo. Dòng được đánh số từ 0 (nội bộ); khi hiển thị cộng 1.
struct Cursor {
    int row;        // chỉ số dòng hiện tại (0 .. lineCount-1)
    int col;        // chỉ số cột hiện tại (0 .. độ dài dòng)
    int wantedCol;  // cột "mong muốn": nhớ cột khi di chuyển lên/xuống qua dòng ngắn

    Cursor() : row(0), col(0), wantedCol(0) {}

    // Đặt con trỏ tại (r, c) và cập nhật luôn wantedCol
    void set(int r, int c) { row = r; col = c; wantedCol = c; }
};