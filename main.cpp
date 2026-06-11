#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <climits>
#include <conio.h>

#ifdef _WIN32
#include <windows.h>
void enableANSI() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#else
void enableANSI() {}
#endif

#include "sequence.hpp"
#include "scoring.hpp"
#include "dp_matrix.hpp"
#include "nw.hpp"
#include "sw.hpp"
#include "greedy.hpp"
#include "renderer.hpp"
#include "confidence.hpp"
#include "exporter.hpp"
#include "config.hpp"

// Recursive multi-traceback path finder
void findTracebackPaths(
    int i, int j,
    const DPMatrix& mat,
    const Sequence& seq1,
    const Sequence& seq2,
    const ScoringConfig& cfg,
    std::vector<std::pair<int, int>>& currentPath,
    std::vector<std::vector<std::pair<int, int>>>& allPaths,
    bool isSW
) {
    if (allPaths.size() >= 3) return; // Limit to 3 paths

    currentPath.push_back({i, j});

    // Base case
    if (isSW) {
        if (mat.at(i, j).score <= 0) {
            allPaths.push_back(currentPath);
            currentPath.pop_back();
            return;
        }
    } else {
        if (i == 0 && j == 0) {
            allPaths.push_back(currentPath);
            currentPath.pop_back();
            return;
        }
    }

    int currentScore = mat.at(i, j).score;
    struct Move {
        int nextI;
        int nextJ;
    };
    std::vector<Move> validMoves;

    if (i > 0 && j > 0) {
        int score = cfg.getScore(seq1[i-1], seq2[j-1]);
        if (currentScore == mat.at(i-1, j-1).score + score) {
            validMoves.push_back({i-1, j-1});
        }
    }
    if (i > 0) {
        if (currentScore == mat.at(i-1, j).score + cfg.gap_open) {
            validMoves.push_back({i-1, j});
        }
    }
    if (j > 0) {
        if (currentScore == mat.at(i, j-1).score + cfg.gap_open) {
            validMoves.push_back({i, j-1});
        }
    }

    // Fallback if no exact match found due to boundary conditions
    if (validMoves.empty()) {
        Direction d = mat.at(i, j).dir;
        if (d == Direction::DIAG && i > 0 && j > 0) {
            validMoves.push_back({i-1, j-1});
        } else if (d == Direction::UP && i > 0) {
            validMoves.push_back({i-1, j});
        } else if (d == Direction::LEFT && j > 0) {
            validMoves.push_back({i, j-1});
        }
    }

    for (const auto& move : validMoves) {
        findTracebackPaths(move.nextI, move.nextJ, mat, seq1, seq2, cfg, currentPath, allPaths, isSW);
        if (allPaths.size() >= 3) break;
    }

    currentPath.pop_back();
}

// Re-computes matrix, traceback, and confidence for the current main view setup
void updateAlignment(AppState& state) {
    bool isSW = (state.currentAlgorithmName == "Smith-Waterman");
    if (isSW) {
        SmithWaterman sw(state.seq1, state.seq2, state.scoringConfig);
        sw.fill();
        ConfidenceAnalyzer::compute(sw.matrix, state.seq1, state.seq2, state.scoringConfig, true);
        state.mainMatrix = sw.matrix;
        state.mainResult = sw.traceback();
        
        state.tracebackPaths.clear();
        std::vector<std::pair<int, int>> curPath;
        auto [si, sj] = sw.matrix.maxScorePos();
        findTracebackPaths(si, sj, state.mainMatrix, state.seq1, state.seq2, state.scoringConfig, curPath, state.tracebackPaths, true);
    } else {
        NeedlemanWunsch nw(state.seq1, state.seq2, state.scoringConfig);
        nw.fill();
        ConfidenceAnalyzer::compute(nw.matrix, state.seq1, state.seq2, state.scoringConfig, false);
        state.mainMatrix = nw.matrix;
        state.mainResult = nw.traceback();
        
        state.tracebackPaths.clear();
        std::vector<std::pair<int, int>> curPath;
        findTracebackPaths(state.seq1.length(), state.seq2.length(), state.mainMatrix, state.seq1, state.seq2, state.scoringConfig, curPath, state.tracebackPaths, false);
    }
}

// Reads keyboard input (supporting arrow keys on Windows)
int getKeyPress() {
    if (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            return 256 + _getch(); // Arrow codes
        }
        return ch;
    }
    return -1;
}

ScoringConfig configureScoringInteractive() {
    ScoringConfig cfg;
    std::cout << "\nScoring Presets:\n";
    std::cout << "  1. Default (match=2, mismatch=-1, gap=-2)\n";
    std::cout << "  2. Strict  (match=5, mismatch=-4, gap=-6)\n";
    std::cout << "  3. BLOSUM62\n";
    std::cout << "  4. PAM250\n";
    std::cout << "  5. Custom\n";
    std::cout << "Choice: ";
    int c = 1;
    std::cin >> c;

    if (c == 1) { cfg.match=2; cfg.mismatch=-1; cfg.gap_open=-2; cfg.gap_extend=-1; }
    else if (c == 2) { cfg.match=5; cfg.mismatch=-4; cfg.gap_open=-6; cfg.gap_extend=-2; }
    else if (c == 3) { cfg.mode = ScoringMode::BLOSUM62; }
    else if (c == 4) { cfg.mode = ScoringMode::PAM250; }
    else if (c == 5) {
        std::cout << "Match score: ";    std::cin >> cfg.match;
        std::cout << "Mismatch penalty: "; std::cin >> cfg.mismatch;
        std::cout << "Gap open penalty: "; std::cin >> cfg.gap_open;
        std::cout << "Gap extend penalty: "; std::cin >> cfg.gap_extend;
    }
    Config::save(cfg);
    return cfg;
}

int main() {
    enableANSI();

    Sequence seq1, seq2;
    std::cout << "Enter Sequence 1 name: "; std::cin >> seq1.name;
    std::cout << "Enter Sequence 1 (A/T/G/C): "; std::cin >> seq1.data;
    seq1.normalize();

    std::cout << "Enter Sequence 2 name: "; std::cin >> seq2.name;
    std::cout << "Enter Sequence 2 (A/T/G/C): "; std::cin >> seq2.data;
    seq2.normalize();

    try {
        seq1.validate();
        seq2.validate();
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    Renderer renderer;
    renderer.clearScreen();
    renderer.hideCursor();

    AppState state;
    state.seq1 = seq1;
    state.seq2 = seq2;
    Config::load(state.scoringConfig);

    updateAlignment(state);

    bool running = true;
    while (running) {
        renderer.render(state);

        // Sleep briefly to avoid maxing out CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        int key = getKeyPress();
        if (key == -1) continue;

        if (state.mode == AppMode::MAIN) {
            // Main Controls
            if (key == 'q' || key == 'Q' || key == 27) { // 27 = Esc
                running = false;
            } else if (key == 'h' || key == 'H') {
                state.heatmapOverlay = !state.heatmapOverlay;
            } else if (key == 's' || key == 'S') {
                state.sidePanelOpen = !state.sidePanelOpen;
            } else if (key == 't' || key == 'T') {
                if (state.currentAlgorithmName == "Needleman-Wunsch") {
                    state.currentAlgorithmName = "Smith-Waterman";
                } else {
                    state.currentAlgorithmName = "Needleman-Wunsch";
                }
                updateAlignment(state);
            } else if (key == 'e' || key == 'E') {
                state.mode = AppMode::MUTATION;
                state.mutationCursorIdx = 0;
                state.editingSeq2 = false;
                state.showDeltaScore = false;
            } else if (key == 'd' || key == 'D') {
                state.mode = AppMode::DOTPLOT;
                state.dotPlotShowOverlay = false;
            } else if (key == 'r' || key == 'R') {
                // Run Race Mode
                state.mode = AppMode::RACE;
                state.raceCompleted = false;
                state.nwCells = 0;
                state.swCells = 0;
                state.greedyCells = 0;

                int m = state.seq1.length();
                int n = state.seq2.length();

                state.nwMatrix = DPMatrix(m + 1, n + 1);
                state.swMatrix = DPMatrix(m + 1, n + 1);
                state.greedyMatrix = DPMatrix(m + 1, n + 1);

                // Run benchmarks first
                auto t1 = std::chrono::high_resolution_clock::now();
                NeedlemanWunsch nw_bench(state.seq1, state.seq2, state.scoringConfig);
                nw_bench.fill();
                auto t2 = std::chrono::high_resolution_clock::now();
                state.nwTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

                auto t3 = std::chrono::high_resolution_clock::now();
                SmithWaterman sw_bench(state.seq1, state.seq2, state.scoringConfig);
                sw_bench.fill();
                auto t4 = std::chrono::high_resolution_clock::now();
                state.swTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();

                auto t5 = std::chrono::high_resolution_clock::now();
                GreedyAlignment gr_bench(state.seq1, state.seq2, state.scoringConfig);
                gr_bench.fill();
                auto t6 = std::chrono::high_resolution_clock::now();
                state.greedyTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(t6 - t5).count();

                int nwScore = nw_bench.matrix.at(m, n).score;
                int swScore = sw_bench.matrix.maxScore();
                int grScore = gr_bench.matrix.at(m, n).score;

                int bestScore = std::max({nwScore, swScore, grScore});
                if (bestScore == nwScore) state.scoreWinner = "Needleman-Wunsch (Global)";
                else if (bestScore == swScore) state.scoreWinner = "Smith-Waterman (Local)";
                else state.scoreWinner = "Greedy Heuristic";

                long long bestTime = std::min({state.nwTimeUs, state.swTimeUs, state.greedyTimeUs});
                if (bestTime == state.nwTimeUs) state.speedWinner = "Needleman-Wunsch";
                else if (bestTime == state.swTimeUs) state.speedWinner = "Smith-Waterman";
                else state.speedWinner = "Greedy Heuristic";

                // Animation guard
                if (m > 25 || n > 25) {
                    // Instantly show final matrices
                    state.nwMatrix = nw_bench.matrix;
                    state.swMatrix = sw_bench.matrix;
                    state.greedyMatrix = gr_bench.matrix;
                    state.nwCells = (m + 1) * (n + 1);
                    state.swCells = (m + 1) * (n + 1);
                    state.greedyCells = gr_bench.greedyPath.size();
                    state.raceCompleted = true;
                } else {
                    // Interleaved animation
                    // NW Setup
                    for (int i = 0; i <= m; i++) {
                        state.nwMatrix.at(i,0).score = i * state.scoringConfig.gap_open;
                        state.nwMatrix.at(i,0).dir   = Direction::UP;
                    }
                    for (int j = 0; j <= n; j++) {
                        state.nwMatrix.at(0,j).score = j * state.scoringConfig.gap_open;
                        state.nwMatrix.at(0,j).dir   = Direction::LEFT;
                    }
                    state.nwMatrix.at(0,0).dir = Direction::NONE;

                    // SW Setup
                    for (int i = 0; i <= m; i++) state.swMatrix.at(i,0).score = 0;
                    for (int j = 0; j <= n; j++) state.swMatrix.at(0,j).score = 0;

                    // Greedy Setup
                    for (int i = 0; i <= m; i++) {
                        for (int j = 0; j <= n; j++) {
                            state.greedyMatrix.at(i, j).score = -9999;
                        }
                    }
                    state.greedyMatrix.at(0, 0).score = 0;

                    int nw_i = 1, nw_j = 1;
                    int sw_i = 1, sw_j = 1;
                    int gr_path_idx = 0;

                    bool nw_done = false, sw_done = false, gr_done = false;

                    while (!nw_done || !sw_done || !gr_done) {
                        // NW step
                        if (!nw_done) {
                            int diag = state.nwMatrix.at(nw_i-1,nw_j-1).score + state.scoringConfig.getScore(state.seq1[nw_i-1], state.seq2[nw_j-1]);
                            int up   = state.nwMatrix.at(nw_i-1,nw_j).score   + state.scoringConfig.gap_open;
                            int left = state.nwMatrix.at(nw_i,nw_j-1).score   + state.scoringConfig.gap_open;
                            int best = std::max({diag, up, left});
                            state.nwMatrix.at(nw_i,nw_j).score = best;
                            if (best == diag) state.nwMatrix.at(nw_i,nw_j).dir = Direction::DIAG;
                            else if (best == up) state.nwMatrix.at(nw_i,nw_j).dir = Direction::UP;
                            else state.nwMatrix.at(nw_i,nw_j).dir = Direction::LEFT;
                            state.nwCells++;

                            nw_j++;
                            if (nw_j > n) { nw_j = 1; nw_i++; }
                            if (nw_i > m) nw_done = true;
                        }

                        // SW step
                        if (!sw_done) {
                            int diag = state.swMatrix.at(sw_i-1,sw_j-1).score + state.scoringConfig.getScore(state.seq1[sw_i-1], state.seq2[sw_j-1]);
                            int up   = state.swMatrix.at(sw_i-1,sw_j).score   + state.scoringConfig.gap_open;
                            int left = state.swMatrix.at(sw_i,sw_j-1).score   + state.scoringConfig.gap_open;
                            int best = std::max({diag, up, left, 0});
                            state.swMatrix.at(sw_i,sw_j).score = best;
                            if (best == 0) state.swMatrix.at(sw_i,sw_j).dir = Direction::NONE;
                            else if (best == diag) state.swMatrix.at(sw_i,sw_j).dir = Direction::DIAG;
                            else if (best == up) state.swMatrix.at(sw_i,sw_j).dir = Direction::UP;
                            else state.swMatrix.at(sw_i,sw_j).dir = Direction::LEFT;
                            state.swCells++;

                            sw_j++;
                            if (sw_j > n) { sw_j = 1; sw_i++; }
                            if (sw_i > m) sw_done = true;
                        }

                        // Greedy step
                        if (!gr_done) {
                            if (gr_path_idx < (int)gr_bench.greedyPath.size()) {
                                auto p = gr_bench.greedyPath[gr_path_idx];
                                state.greedyMatrix.at(p.first, p.second) = gr_bench.matrix.at(p.first, p.second);
                                state.greedyCells++;
                                gr_path_idx++;
                            } else {
                                gr_done = true;
                            }
                        }

                        renderer.render(state);
                        std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    }
                    state.raceCompleted = true;
                }
            } else if (key == 'c' || key == 'C') {
                renderer.showCursor();
                renderer.clearScreen();
                state.scoringConfig = configureScoringInteractive();
                updateAlignment(state);
                renderer.hideCursor();
            } else if (key == 'x' || key == 'X') {
                renderer.showCursor();
                renderer.clearScreen();
                bool ok = Exporter::save(state.mainResult, state.currentAlgorithmName, state.seq1.name, state.seq2.name);
                if (ok) {
                    std::cout << "\n\033[1;32mSaved alignment result to alignment_result.txt successfully!\033[0m\n";
                } else {
                    std::cout << "\n\033[1;31mFailed to save alignment result.\033[0m\n";
                }
                std::cout << "Press any key to return...";
                _getch();
                renderer.hideCursor();
            } else {
                // Navigation (WASD or Arrow Keys)
                int rMax = state.seq1.length();
                int cMax = state.seq2.length();
                if (key == 'w' || key == 'W' || key == 328) {
                    if (state.hoverRow > 0) state.hoverRow--;
                } else if (key == 's' || key == 'S' || key == 336) {
                    if (state.hoverRow < rMax) state.hoverRow++;
                } else if (key == 'a' || key == 'A' || key == 331) {
                    if (state.hoverCol > 0) state.hoverCol--;
                } else if (key == 'd' || key == 'D' || key == 333) {
                    if (state.hoverCol < cMax) state.hoverCol++;
                }
            }
        } else if (state.mode == AppMode::MUTATION) {
            // Mutation Controls
            if (key == 27) { // Esc
                state.mode = AppMode::MAIN;
                state.showDeltaScore = false;
            } else if (key == 13) { // Enter
                state.mode = AppMode::MAIN;
            } else if (key == 331) { // Left arrow
                if (state.mutationCursorIdx > 0) state.mutationCursorIdx--;
            } else if (key == 333) { // Right arrow
                int maxLen = state.editingSeq2 ? state.seq2.length() : state.seq1.length();
                if (state.mutationCursorIdx < maxLen - 1) state.mutationCursorIdx++;
            } else if (key == 328) { // Up arrow (switch to Seq 1)
                state.editingSeq2 = false;
                state.mutationCursorIdx = std::min(state.mutationCursorIdx, state.seq1.length() - 1);
            } else if (key == 336) { // Down arrow (switch to Seq 2)
                state.editingSeq2 = true;
                state.mutationCursorIdx = std::min(state.mutationCursorIdx, state.seq2.length() - 1);
            } else {
                char mutationChar = toupper(key);
                if (mutationChar == 'A' || mutationChar == 'T' || mutationChar == 'G' || mutationChar == 'C') {
                    char oldChar = state.editingSeq2 ? state.seq2[state.mutationCursorIdx] : state.seq1[state.mutationCursorIdx];
                    if (mutationChar != oldChar) {
                        DPMatrix oldMat = state.mainMatrix;
                        int oldScore = state.mainResult.score;

                        if (state.editingSeq2) {
                            state.seq2.data[state.mutationCursorIdx] = mutationChar;
                        } else {
                            state.seq1.data[state.mutationCursorIdx] = mutationChar;
                        }

                        updateAlignment(state);

                        // Identify affected cells
                        state.affectedCells.clear();
                        for (int r = 0; r <= state.seq1.length(); ++r) {
                            for (int c = 0; c <= state.seq2.length(); ++c) {
                                if (oldMat.at(r, c).score != state.mainMatrix.at(r, c).score) {
                                    state.affectedCells.push_back({r, c});
                                }
                            }
                        }

                        state.deltaScore = state.mainResult.score - oldScore;
                        state.showDeltaScore = true;
                        state.flashFramesLeft = 3;

                        // Flash cells for 3 frames
                        while (state.flashFramesLeft > 0) {
                            renderer.render(state);
                            std::this_thread::sleep_for(std::chrono::milliseconds(150));
                            state.flashFramesLeft--;
                        }
                    }
                }
            }
        } else if (state.mode == AppMode::RACE) {
            // Race Controls
            if (key == 27 || key == 'q' || key == 'Q') { // Esc or Q
                state.mode = AppMode::MAIN;
            }
        } else if (state.mode == AppMode::DOTPLOT) {
            // Dot Plot Controls
            if (key == 'o' || key == 'O') {
                state.dotPlotShowOverlay = !state.dotPlotShowOverlay;
            } else if (key == 27 || key == 'q' || key == 'Q') { // Esc or Q
                state.mode = AppMode::MAIN;
            }
        }
    }

    renderer.clearScreen();
    renderer.showCursor();
    return 0;
}