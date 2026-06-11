#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "dp_matrix.hpp"
#include "sequence.hpp"
#include "nw.hpp"

// ANSI color codes (color-blind safe: blue/orange palette)
namespace Color {
    const std::string RESET   = "\033[0m";
    const std::string BOLD    = "\033[1m";

    // Color-blind safe palette
    const std::string HIGH_CONF  = "\033[38;5;33m";   // blue  - certain
    const std::string MED_CONF   = "\033[38;5;214m";  // orange - moderate
    const std::string LOW_CONF   = "\033[38;5;250m";  // grey  - uncertain

    const std::string POSITIVE   = "\033[38;5;33m";   // blue
    const std::string NEGATIVE   = "\033[38;5;208m";  // orange
    const std::string ZERO       = "\033[38;5;250m";  // grey

    const std::string ACTIVE     = "\033[48;5;226m\033[38;5;0m";  // yellow bg black fg
    const std::string TRACEBACK  = "\033[48;5;33m\033[38;5;255m"; // blue bg white fg
    const std::string HEADER     = "\033[38;5;255m\033[1m";

    const std::string CURSOR_HIDE = "\033[?25l";
    const std::string CURSOR_SHOW = "\033[?25h";
    const std::string CLEAR       = "\033[2J\033[H";
}

namespace Cursor {
    std::string moveTo(int row, int col) {
        return "\033[" + std::to_string(row) + ";" + std::to_string(col) + "H";
    }
    std::string moveUp(int n)    { return "\033[" + std::to_string(n) + "A"; }
    std::string moveDown(int n)  { return "\033[" + std::to_string(n) + "B"; }
    std::string moveRight(int n) { return "\033[" + std::to_string(n) + "C"; }
    std::string moveLeft(int n)  { return "\033[" + std::to_string(n) + "D"; }
}

class Renderer {
public:
    int cellWidth = 5;

    void clearScreen() {
        std::cout << Color::CLEAR;
    }

    void hideCursor() { std::cout << Color::CURSOR_HIDE; }
    void showCursor() { std::cout << Color::CURSOR_SHOW; }

    std::string colorForScore(int score) {
        if (score > 0) return Color::POSITIVE;
        if (score < 0) return Color::NEGATIVE;
        return Color::ZERO;
    }

    std::string colorForConfidence(float conf) {
        if (conf > 0.6f) return Color::HIGH_CONF;
        if (conf > 0.3f) return Color::MED_CONF;
        return Color::LOW_CONF;
    }

    void drawMatrix(
        const DPMatrix& mat,
        const Sequence& seq1,
        const Sequence& seq2,
        const std::vector<std::pair<int,int>>& traceback = {},
        int activeRow = -1, int activeCol = -1,
        bool showConfidence = false,
        int startRow = 1, int startCol = 1
    ) {
        int m = mat.rows;
        int n = mat.cols;

        // header row: seq2 letters
        std::cout << Cursor::moveTo(startRow, startCol);
        std::cout << Color::HEADER;
        std::cout << std::string(cellWidth, ' '); // top-left corner
        std::cout << std::string(cellWidth, ' '); // gap column header
        for (int j = 0; j < seq2.length(); j++) {
            std::string label = "  " + std::string(1, seq2[j]) + "  ";
            label = label.substr(0, cellWidth);
            std::cout << label;
        }
        std::cout << Color::RESET << "\n";

        for (int i = 0; i < m; i++) {
            std::cout << Cursor::moveTo(startRow + 1 + i, startCol);

            // row header: seq1 letter
            std::cout << Color::HEADER;
            if (i == 0) std::cout << std::string(cellWidth, ' ');
            else {
                std::string label = "  " + std::string(1, seq1[i-1]) + "  ";
                label = label.substr(0, cellWidth);
                std::cout << label;
            }
            std::cout << Color::RESET;

            for (int j = 0; j < n; j++) {
                const Cell& cell = mat.at(i, j);
                bool isActive   = (i == activeRow && j == activeCol);
                bool isTraceback = false;
                for (auto& p : traceback)
                    if (p.first == i && p.second == j) { isTraceback = true; break; }

                std::string colorCode;
                if (isActive)        colorCode = Color::ACTIVE;
                else if (isTraceback) colorCode = Color::TRACEBACK;
                else if (showConfidence) colorCode = colorForConfidence(cell.confidence);
                else                  colorCode = colorForScore(cell.score);

                std::string val = std::to_string(cell.score);
                // pad to cellWidth
                while ((int)val.size() < cellWidth - 1) val = " " + val;
                val = " " + val;
                val = val.substr(0, cellWidth);

                std::cout << colorCode << val << Color::RESET;
            }
            std::cout << "\n";
        }
    }

    void drawSidePanel(int row, int col, int diagVal, int upVal, int leftVal, int chosen) {
        std::cout << Cursor::moveTo(row, col);
        std::cout << Color::HEADER << "=== Cell Candidates ===" << Color::RESET << "\n";
        std::cout << Cursor::moveTo(row+1, col);

        auto mark = [&](int val, const std::string& label) {
            bool isChosen = (val == chosen);
            std::string prefix = isChosen ? " >> " : "    ";
            std::string color  = isChosen ? Color::TRACEBACK : Color::RESET;
            std::cout << color << prefix << label << ": " << val << Color::RESET << "\n";
        };

        mark(diagVal, "DIAG (match/mismatch)");
        std::cout << Cursor::moveTo(row+2, col);
        mark(upVal,   "UP   (gap in seq2)   ");
        std::cout << Cursor::moveTo(row+3, col);
        mark(leftVal, "LEFT (gap in seq1)   ");
        std::cout << Cursor::moveTo(row+4, col);
        std::cout << Color::BOLD << "    Best: " << chosen << Color::RESET << "\n";
    }

    void drawAlignmentResult(const AlignmentResult& res, const std::string& algoName, int row, int col) {
        std::cout << Cursor::moveTo(row, col);
        std::cout << Color::HEADER << "=== " << algoName << " Result ===" << Color::RESET << "\n";
        std::cout << Cursor::moveTo(row+1, col) << "Seq1: " << res.aligned1 << "\n";
        std::cout << Cursor::moveTo(row+2, col) << "      " << res.matchLine << "\n";
        std::cout << Cursor::moveTo(row+3, col) << "Seq2: " << res.aligned2 << "\n";
        std::cout << Cursor::moveTo(row+4, col)
                  << "Score: " << res.score
                  << "  Matches: " << res.matches
                  << "  Mismatches: " << res.mismatches
                  << "  Gaps: " << res.gaps
                  << "  Identity: " << res.identity << "%\n";
    }

    void drawDotPlot(const Sequence& s1, const Sequence& s2,
                     const std::vector<std::pair<int,int>>& alignPath,
                     int startRow, int startCol) {
        int m = s1.length();
        int n = s2.length();

        std::cout << Cursor::moveTo(startRow, startCol);
        std::cout << Color::HEADER << "=== Dot Plot ===" << Color::RESET;

        // header
        std::cout << Cursor::moveTo(startRow+1, startCol+3);
        for (int j = 0; j < n; j++) std::cout << s2[j] << " ";

        for (int i = 0; i < m; i++) {
            std::cout << Cursor::moveTo(startRow+2+i, startCol);
            std::cout << Color::HEADER << s1[i] << " " << Color::RESET;

            for (int j = 0; j < n; j++) {
                bool onPath = false;
                for (auto& p : alignPath)
                    if (p.first == i+1 && p.second == j+1) { onPath = true; break; }

                if (onPath) {
                    std::cout << Color::TRACEBACK << "* " << Color::RESET;
                } else if (s1[i] == s2[j]) {
                    std::cout << Color::POSITIVE << ". " << Color::RESET;
                } else {
                    std::cout << "  ";
                }
            }
        }
    }
};