#pragma once
#include <vector>
#include <string>
#include <climits>

enum class Direction { NONE, DIAG, UP, LEFT, DIAG_UP, DIAG_LEFT, UP_LEFT, ALL };

struct Cell {
    int score = 0;
    Direction dir = Direction::NONE;
    float confidence = 0.0f; // 0.0 = uncertain, 1.0 = certain
};

struct DPMatrix {
    int rows, cols;
    std::vector<std::vector<Cell>> table;

    DPMatrix() : rows(0), cols(0) {}

    DPMatrix(int r, int c) : rows(r), cols(c) {
        table.assign(r, std::vector<Cell>(c));
    }

    Cell& at(int i, int j) { return table[i][j]; }
    const Cell& at(int i, int j) const { return table[i][j]; }

    void reset() {
        for (auto& row : table)
            for (auto& cell : row) {
                cell.score = 0;
                cell.dir = Direction::NONE;
                cell.confidence = 0.0f;
            }
    }

    int maxScore() const {
        int mx = INT_MIN;
        for (auto& row : table)
            for (auto& c : row)
                mx = std::max(mx, c.score);
        return mx;
    }

    std::pair<int,int> maxScorePos() const {
        int mx = INT_MIN;
        std::pair<int,int> pos = {0,0};
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++)
                if (table[i][j].score > mx) {
                    mx = table[i][j].score;
                    pos = {i, j};
                }
        return pos;
    }
};