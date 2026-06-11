#pragma once
#include "nw.hpp"
#include "sw.hpp"
#include "renderer.hpp"
#include "sequence.hpp"
#include "scoring.hpp"
#include <chrono>
#include <thread>
#include <iostream>

class RaceMode {
public:
    const Sequence& seq1;
    const Sequence& seq2;
    const ScoringConfig& config;
    Renderer renderer;

    // stats
    struct Stats {
        std::string name;
        int score = 0;
        long long timeUs = 0;
        int cellsComputed = 0;
        int memoryBytes = 0;
    };

    RaceMode(const Sequence& s1, const Sequence& s2, const ScoringConfig& cfg)
        : seq1(s1), seq2(s2), config(cfg) {}

    void run() {
        renderer.clearScreen();
        renderer.hideCursor();

        int m = seq1.length();
        int n = seq2.length();

        // panel column offsets
        int nwCol = 2;
        int swCol = (n+3) * 5 + 4;

        // draw panel headers
        std::cout << Cursor::moveTo(1, nwCol);
        std::cout << Color::HEADER << "  NEEDLEMAN-WUNSCH (Global)" << Color::RESET;
        std::cout << Cursor::moveTo(1, swCol);
        std::cout << Color::HEADER << "  SMITH-WATERMAN (Local)" << Color::RESET;

        NeedlemanWunsch nw(seq1, seq2, config);
        SmithWaterman   sw(seq1, seq2, config);

        Stats nwStats, swStats;
        nwStats.name = "Needleman-Wunsch";
        swStats.name = "Smith-Waterman";

        int nwCells = 0, swCells = 0;

        // set callbacks to draw each cell as it fills
        nw.onStep = [&](int i, int j, int score, int d, int u, int l) {
            nwCells++;
            std::cout << Cursor::moveTo(i+2, nwCol + j*5);
            std::string val = std::to_string(score);
            while ((int)val.size() < 4) val = " " + val;
            std::cout << Color::POSITIVE << val << Color::RESET;
            std::cout.flush();
        };

        sw.onStep = [&](int i, int j, int score, int d, int u, int l) {
            swCells++;
            std::cout << Cursor::moveTo(i+2, swCol + j*5);
            std::string val = std::to_string(score);
            while ((int)val.size() < 4) val = " " + val;
            std::cout << (score > 0 ? Color::POSITIVE : Color::ZERO) << val << Color::RESET;
            std::cout.flush();
        };

        // run NW with timing
        auto t1 = std::chrono::high_resolution_clock::now();
        nw.fill();
        auto t2 = std::chrono::high_resolution_clock::now();
        nwStats.timeUs = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1).count();
        nwStats.cellsComputed = nwCells;
        nwStats.score = nw.matrix.at(m, n).score;
        nwStats.memoryBytes = (m+1)*(n+1)*sizeof(Cell);

        // run SW with timing
        auto t3 = std::chrono::high_resolution_clock::now();
        sw.fill();
        auto t4 = std::chrono::high_resolution_clock::now();
        swStats.timeUs = std::chrono::duration_cast<std::chrono::microseconds>(t4-t3).count();
        swStats.cellsComputed = swCells;
        swStats.score = sw.matrix.maxScore();
        swStats.memoryBytes = (m+1)*(n+1)*sizeof(Cell);

        // summary bar at bottom
        int summaryRow = m + 5;
        drawSummary(nwStats, swStats, summaryRow);

        renderer.showCursor();
    }

private:
    void drawSummary(const Stats& nw, const Stats& sw, int row) {
        std::cout << Cursor::moveTo(row, 1);
        std::cout << Color::HEADER
                  << "==================== RACE SUMMARY ====================" 
                  << Color::RESET << "\n";

        std::cout << Cursor::moveTo(row+1, 1);
        std::cout << std::left;
        std::cout << Color::BOLD << std::string(20, ' ')
                  << "NW" << std::string(15, ' ')
                  << "SW" << Color::RESET << "\n";

        auto printRow = [&](const std::string& label, 
                            const std::string& nwVal, 
                            const std::string& swVal, int r) {
            std::cout << Cursor::moveTo(r, 1);
            std::cout << label << std::string(20 - label.size(), ' ')
                      << nwVal << std::string(17 - nwVal.size(), ' ')
                      << swVal << "\n";
        };

        printRow("Score",
            std::to_string(nw.score),
            std::to_string(sw.score),
            row+2);

        printRow("Time (us)",
            std::to_string(nw.timeUs),
            std::to_string(sw.timeUs),
            row+3);

        printRow("Cells computed",
            std::to_string(nw.cellsComputed),
            std::to_string(sw.cellsComputed),
            row+4);

        printRow("Memory (bytes)",
            std::to_string(nw.memoryBytes),
            std::to_string(sw.memoryBytes),
            row+5);

        std::cout << Cursor::moveTo(row+7, 1);
        std::cout << Color::HEADER
                  << "Interpretation: NW aligns full sequences (higher score for similar length).\n"
                  << "SW finds best local region (better when one sequence is a fragment)."
                  << Color::RESET << "\n";
    }
};