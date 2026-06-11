#include "renderer.hpp"
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <cmath>

// Helper to determine if a cell is part of a diagonal streak
bool isDiagonalStreak(const Sequence& s1, const Sequence& s2, int i, int j) {
    if (s1[i] != s2[j]) return false;
    if (i > 0 && j > 0 && s1[i-1] == s2[j-1]) return true;
    if (i < s1.length() - 1 && j < s2.length() - 1 && s1[i+1] == s2[j+1]) return true;
    return false;
}

void Renderer::clearScreen() const {
    std::cout << "\033[2J\033[H";
}

void Renderer::hideCursor() const {
    std::cout << "\033[?25l";
}

void Renderer::showCursor() const {
    std::cout << "\033[?25h";
}

std::string Renderer::formatCell(int score, Direction dir, bool isUncomputed) const {
    if (isUncomputed) {
        return "    ";
    }
    std::string scoreStr = std::to_string(score);
    while (scoreStr.length() < 3) {
        scoreStr = " " + scoreStr;
    }
    std::string arrow = " ";
    switch (dir) {
        case Direction::DIAG: arrow = "↖"; break;
        case Direction::UP:   arrow = "↑"; break;
        case Direction::LEFT: arrow = "←"; break;
        default: arrow = " "; break;
    }
    return scoreStr + arrow;
}

std::string Renderer::getCellColor(const AppState& state, int r, int c) const {
    // 1. Flash effect
    if (state.flashFramesLeft > 0) {
        bool isAffected = false;
        for (const auto& p : state.affectedCells) {
            if (p.first == r && p.second == c) {
                isAffected = true;
                break;
            }
        }
        if (isAffected) {
            return "\033[7;1m"; // Inverted bold
        }
    }

    // 2. Active / Hovered cell
    if (r == state.hoverRow && c == state.hoverCol) {
        return "\033[48;5;226m\033[38;5;0m"; // Yellow bg, black fg
    }

    // 3. Traceback paths
    for (size_t pIdx = 0; pIdx < state.tracebackPaths.size(); ++pIdx) {
        for (const auto& p : state.tracebackPaths[pIdx]) {
            if (p.first == r && p.second == c) {
                if (pIdx == 0) return config.tracebackColor1;
                if (pIdx == 1) return config.tracebackColor2;
                if (pIdx == 2) return config.tracebackColor3;
            }
        }
    }

    // 4. Heatmap overlay
    if (state.heatmapOverlay) {
        const Cell& cell = state.mainMatrix.at(r, c);
        if (cell.confidence <= 0.3f) {
            return config.heatmapLow;
        } else if (cell.confidence <= 0.6f) {
            return config.heatmapMid;
        } else {
            return config.heatmapHigh;
        }
    }

    // 5. Default score colors
    const Cell& cell = state.mainMatrix.at(r, c);
    if (cell.score == -9999) {
        return "\033[0m"; // Uncomputed
    }
    if (cell.score > 0) return config.matchColor;
    if (cell.score < 0) return config.mismatchColor;
    return config.gapColor;
}

void Renderer::drawMainView(const AppState& state) {
    int m = state.seq1.length();
    int n = state.seq2.length();

    // Sequence editing header in Mutation Mode
    if (state.mode == AppMode::MUTATION) {
        std::cout << "\033[1;33m=== MUTATION MODE ===\033[0m\n";
        std::cout << "Seq 1 (Vertical):   ";
        for (int i = 0; i < state.seq1.length(); ++i) {
            if (!state.editingSeq2 && i == state.mutationCursorIdx) {
                std::cout << "\033[7;1;36m" << state.seq1[i] << "\033[0m ";
            } else {
                std::cout << state.seq1[i] << " ";
            }
        }
        std::cout << "\nSeq 2 (Horizontal): ";
        for (int i = 0; i < state.seq2.length(); ++i) {
            if (state.editingSeq2 && i == state.mutationCursorIdx) {
                std::cout << "\033[7;1;36m" << state.seq2[i] << "\033[0m ";
            } else {
                std::cout << state.seq2[i] << " ";
            }
        }
        std::cout << "\n\n";
    }

    // Grid Column Headers
    std::cout << "    "; // corner spacer
    for (int j = 0; j < n; ++j) {
        std::cout << " " << state.seq2[j] << "  ";
    }
    std::cout << "\n";

    // Main Grid Rows
    for (int i = 0; i <= m; ++i) {
        // Row header
        if (i == 0) std::cout << "  ";
        else std::cout << state.seq1[i-1] << " ";
        std::cout << "│";

        for (int j = 0; j <= n; ++j) {
            const Cell& cell = state.mainMatrix.at(i, j);
            std::string color = getCellColor(state, i, j);
            std::cout << color << formatCell(cell.score, cell.dir, cell.score == -9999) << "\033[0m";
        }

        // Side panel (toggleable) next to grid rows
        if (state.sidePanelOpen) {
            int gridRightCol = (n + 2) * 4 + 4;
            if (i == 1) {
                std::cout << "\033[" << std::to_string(i + (state.mode == AppMode::MUTATION ? 5 : 2)) << ";" << std::to_string(gridRightCol) << "H";
                std::cout << "\033[1;37m  === Hover Info ===\033[0m";
            } else if (i == 2) {
                std::cout << "\033[" << std::to_string(i + (state.mode == AppMode::MUTATION ? 5 : 2)) << ";" << std::to_string(gridRightCol) << "H";
                std::cout << "  Cell: (" << state.hoverRow << ", " << state.hoverCol << ")";
            } else if (i == 3) {
                std::cout << "\033[" << std::to_string(i + (state.mode == AppMode::MUTATION ? 5 : 2)) << ";" << std::to_string(gridRightCol) << "H";
                if (state.hoverRow > 0 && state.hoverCol > 0 && state.hoverRow <= m && state.hoverCol <= n) {
                    char c1 = state.seq1[state.hoverRow - 1];
                    char c2 = state.seq2[state.hoverCol - 1];
                    std::cout << "  Symbols: " << c1 << " vs " << c2;
                } else {
                    std::cout << "  Symbols: - vs -";
                }
            } else if (i == 4) {
                std::cout << "\033[" << std::to_string(i + (state.mode == AppMode::MUTATION ? 5 : 2)) << ";" << std::to_string(gridRightCol) << "H";
                if (state.hoverRow > 0 && state.hoverCol > 0 && state.hoverRow <= m && state.hoverCol <= n) {
                    char c1 = state.seq1[state.hoverRow - 1];
                    char c2 = state.seq2[state.hoverCol - 1];
                    int blosumVal = ScoringConfig::BLOSUM62[ScoringConfig::charToIdx(c1)][ScoringConfig::charToIdx(c2)];
                    std::cout << "  BLOSUM62: " << (blosumVal >= 0 ? "+" : "") << blosumVal;
                } else {
                    std::cout << "  BLOSUM62: N/A";
                }
            } else if (i == 5) {
                std::cout << "\033[" << std::to_string(i + (state.mode == AppMode::MUTATION ? 5 : 2)) << ";" << std::to_string(gridRightCol) << "H";
                if (state.hoverRow > 0 && state.hoverCol > 0 && state.hoverRow <= m && state.hoverCol <= n) {
                    char c1 = state.seq1[state.hoverRow - 1];
                    char c2 = state.seq2[state.hoverCol - 1];
                    int pamVal = ScoringConfig::PAM250[ScoringConfig::charToIdx(c1)][ScoringConfig::charToIdx(c2)];
                    std::cout << "  PAM250:   " << (pamVal >= 0 ? "+" : "") << pamVal;
                } else {
                    std::cout << "  PAM250:   N/A";
                }
            }
        }

        std::cout << "\n";
    }
    std::cout << "\n";

    // Alignment Result Visualization
    if (state.mode == AppMode::MAIN && !state.mainResult.aligned1.empty()) {
        std::cout << "\033[1;35m=== Alignment Result ===\033[0m\n";
        std::cout << "Seq1: ";
        for (size_t k = 0; k < state.mainResult.aligned1.length(); ++k) {
            char c1 = state.mainResult.aligned1[k];
            char c2 = state.mainResult.aligned2[k];
            if (c1 == '-' || c2 == '-') std::cout << "\033[37m" << c1 << "\033[0m"; // white gap
            else if (c1 == c2) std::cout << "\033[34m" << c1 << "\033[0m"; // blue match
            else std::cout << "\033[38;5;208m" << c1 << "\033[0m"; // orange mismatch
        }
        std::cout << "\n      ";
        for (size_t k = 0; k < state.mainResult.matchLine.length(); ++k) {
            char matchChar = state.mainResult.matchLine[k];
            if (matchChar == '|') std::cout << "\033[34m|\033[0m";
            else if (matchChar == '.') std::cout << "\033[38;5;208m.\033[0m";
            else std::cout << " ";
        }
        std::cout << "\nSeq2: ";
        for (size_t k = 0; k < state.mainResult.aligned2.length(); ++k) {
            char c1 = state.mainResult.aligned1[k];
            char c2 = state.mainResult.aligned2[k];
            if (c1 == '-' || c2 == '-') std::cout << "\033[37m" << c2 << "\033[0m";
            else if (c1 == c2) std::cout << "\033[34m" << c2 << "\033[0m";
            else std::cout << "\033[38;5;208m" << c2 << "\033[0m";
        }
        std::cout << "\n";
    }
}

void Renderer::drawRaceView(const AppState& state) {
    int m = state.seq1.length();
    int n = state.seq2.length();
    
    // Panel title width tracking
    int cellBlockWidth = (n + 2) * 4;
    
    std::cout << "\033[1;35mNEEDLEMAN-WUNSCH\033[0m";
    int pad1 = std::max(2, cellBlockWidth - 14);
    std::cout << std::string(pad1, ' ') << "│ \033[1;35mSMITH-WATERMAN\033[0m";
    int pad2 = std::max(2, cellBlockWidth - 12);
    std::cout << std::string(pad2, ' ') << "│ \033[1;35mGREEDY ALIGNMENT\033[0m\n";

    auto drawHeader = [&](const Sequence& seq) {
        std::cout << "    ";
        for (int j = 0; j < seq.length(); ++j) {
            std::cout << " " << seq[j] << "  ";
        }
    };
    drawHeader(state.seq2);
    std::cout << " │";
    drawHeader(state.seq2);
    std::cout << " │";
    drawHeader(state.seq2);
    std::cout << "\n";

    for (int i = 0; i <= m; ++i) {
        // NW
        if (i == 0) std::cout << "  ";
        else std::cout << state.seq1[i-1] << " ";
        std::cout << "│";
        for (int j = 0; j <= n; ++j) {
            const Cell& c = state.nwMatrix.at(i, j);
            bool isUncomputed = (c.score == -9999);
            std::string color = isUncomputed ? "\033[0m" : ((c.score > 0) ? "\033[34m" : ((c.score < 0) ? "\033[38;5;208m" : "\033[37m"));
            std::cout << color << formatCell(c.score, c.dir, isUncomputed) << "\033[0m";
        }

        std::cout << " │";

        // SW
        if (i == 0) std::cout << "  ";
        else std::cout << state.seq1[i-1] << " ";
        std::cout << "│";
        for (int j = 0; j <= n; ++j) {
            const Cell& c = state.swMatrix.at(i, j);
            bool isUncomputed = (c.score == -9999);
            std::string color = isUncomputed ? "\033[0m" : ((c.score > 0) ? "\033[34m" : "\033[37m");
            std::cout << color << formatCell(c.score, c.dir, isUncomputed) << "\033[0m";
        }

        std::cout << " │";

        // Greedy
        if (i == 0) std::cout << "  ";
        else std::cout << state.seq1[i-1] << " ";
        std::cout << "│";
        for (int j = 0; j <= n; ++j) {
            const Cell& c = state.greedyMatrix.at(i, j);
            bool isUncomputed = (c.score == -9999);
            std::string color = isUncomputed ? "\033[0m" : ((c.score > 0) ? "\033[34m" : ((c.score < 0) ? "\033[38;5;208m" : "\033[37m"));
            std::cout << color << formatCell(c.score, c.dir, isUncomputed) << "\033[0m";
        }

        std::cout << "\n";
    }

    auto getMemKB = [&](int rows, int cols) {
        return (double)(rows * cols * sizeof(Cell)) / 1024.0;
    };

    char nwBar[128], swBar[128], greedyBar[128];
    sprintf(nwBar, "Score: %d | Cells: %d | Mem: %.2f KB", 
            state.raceCompleted ? state.nwMatrix.at(m, n).score : 0, state.nwCells, getMemKB(m+1, n+1));
    sprintf(swBar, "Score: %d | Cells: %d | Mem: %.2f KB", 
            state.raceCompleted ? state.swMatrix.maxScore() : 0, state.swCells, getMemKB(m+1, n+1));
    sprintf(greedyBar, "Score: %d | Cells: %d | Mem: %.2f KB", 
            state.raceCompleted ? state.greedyMatrix.at(m, n).score : 0, state.greedyCells, getMemKB(m+1, n+1));

    std::cout << "\033[1;36m" << nwBar << "\033[0m";
    int barPad1 = std::max(2, cellBlockWidth + 4 - (int)strlen(nwBar));
    std::cout << std::string(barPad1, ' ') << "│ \033[1;36m" << swBar << "\033[0m";
    int barPad2 = std::max(2, cellBlockWidth + 4 - (int)strlen(swBar));
    std::cout << std::string(barPad2, ' ') << "│ \033[1;36m" << greedyBar << "\033[0m\n";

    if (state.raceCompleted) {
        std::cout << "\n\033[1;32m=== RACE SUMMARY ===\033[0m\n";
        std::cout << "Winner by Score: " << state.scoreWinner << "\n";
        std::cout << "Winner by Speed: " << state.speedWinner << "\n";
    }
}

void Renderer::drawDotPlotView(const AppState& state) {
    int m = state.seq1.length();
    int n = state.seq2.length();

    std::cout << "\033[1;35m=== DOT PLOT " << (state.dotPlotShowOverlay ? "(WITH ALIGNMENT OVERLAY)" : "") << " ===\033[0m\n\n";

    // Column Headers
    std::cout << "    ";
    for (int j = 0; j < n; ++j) {
        std::cout << " " << state.seq2[j];
    }
    std::cout << "\n";

    // Separator
    std::cout << "   ┌";
    for (int j = 0; j < n; ++j) std::cout << "──";
    std::cout << "\n";

    for (int i = 0; i < m; ++i) {
        std::cout << " " << state.seq1[i] << " │";
        for (int j = 0; j < n; ++j) {
            bool onPath = false;
            if (state.dotPlotShowOverlay && !state.tracebackPaths.empty()) {
                for (const auto& p : state.tracebackPaths[0]) {
                    if (p.first == i + 1 && p.second == j + 1) {
                        onPath = true;
                        break;
                    }
                }
            }

            if (onPath) {
                std::cout << " \033[1;36m█\033[0m"; // Alignment path cell: solid block in cyan
            } else if (state.seq1[i] == state.seq2[j]) {
                if (isDiagonalStreak(state.seq1, state.seq2, i, j)) {
                    std::cout << " \033[34m·\033[0m"; // Streak: blue dot
                } else {
                    std::cout << " \033[37m·\033[0m"; // Isolated: white/grey dot
                }
            } else {
                std::cout << "  ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

void Renderer::drawStatusBar(const AppState& state) {
    std::cout << "\033[1;37m" << std::string(80, '=') << "\033[0m\n";
    
    if (state.mode == AppMode::MAIN) {
        std::cout << " \033[1;32mAlgorithm: \033[0m" << state.currentAlgorithmName;
        std::cout << " | \033[1;32mScore: \033[0m" << state.mainResult.score;
        std::cout << " | \033[1;32mSeq1: \033[0m" << state.seq1.name << " (" << state.seq1.length() << " chars)";
        std::cout << " | \033[1;32mSeq2: \033[0m" << state.seq2.name << " (" << state.seq2.length() << " chars)\n";
        std::cout << " Controls: [Arrows/WASD] Hover | [H] Heatmap | [E] Mutate | [R] Race | [D] Dot Plot | [S] SidePanel | [Q] Exit\n";
    } else if (state.mode == AppMode::MUTATION) {
        std::cout << " \033[1;33mEditing Sequence\033[0m";
        if (state.showDeltaScore) {
            std::cout << " | \033[1;35mScore changed: \033[0m" << (state.deltaScore >= 0 ? "+" : "") << state.deltaScore;
        }
        std::cout << "\n Controls: [Arrows] Move | [A/T/G/C] Mutate | [Enter/Esc] Finish Editing\n";
    } else if (state.mode == AppMode::RACE) {
        std::cout << " \033[1;33mRACE MODE\033[0m";
        if (state.raceCompleted) {
            std::cout << " | Race Complete!";
        } else {
            std::cout << " | Running Alignment Algorithms...";
        }
        std::cout << "\n Controls: [Esc/Q] Back to Main View\n";
    } else if (state.mode == AppMode::DOTPLOT) {
        std::cout << " \033[1;33mDOT PLOT MODE\033[0m | overlay=" << (state.dotPlotShowOverlay ? "ON" : "OFF");
        std::cout << "\n Controls: [O] Toggle Alignment Path | [Esc/Q] Back to Main View\n";
    }
}

void Renderer::render(const AppState& state) {
    clearScreen();
    hideCursor();
    
    // Draw the active mode's layout
    switch (state.mode) {
        case AppMode::RACE:
            drawRaceView(state);
            break;
        case AppMode::DOTPLOT:
            drawDotPlotView(state);
            break;
        default:
            drawMainView(state);
            break;
    }
    
    // Always draw status bar
    drawStatusBar(state);
}

// -------------------------------------------------------------
// Legacy Wrapper Functions to maintain header interface compatibility
// -------------------------------------------------------------
void Renderer::drawMatrix(
    const DPMatrix& mat,
    const Sequence& seq1,
    const Sequence& seq2,
    const std::vector<std::pair<int,int>>& traceback,
    int activeRow, int activeCol,
    bool showConfidence,
    int startRow, int startCol
) {
    AppState state;
    state.seq1 = seq1;
    state.seq2 = seq2;
    state.mainMatrix = mat;
    state.hoverRow = activeRow;
    state.hoverCol = activeCol;
    state.heatmapOverlay = showConfidence;
    if (!traceback.empty()) {
        state.tracebackPaths.push_back(traceback);
    }
    state.sidePanelOpen = false;
    render(state);
}

void Renderer::drawSidePanel(int row, int col, int diagVal, int upVal, int leftVal, int chosen) {
    // Simply printcandidate summary on a static console row/col for legacy code compatibility
    std::cout << "\033[" << row << ";" << col << "H\033[1mCandidates:\033[0m D:" << diagVal << " U:" << upVal << " L:" << leftVal << " Best:" << chosen;
}

void Renderer::drawAlignmentResult(const AlignmentResult& res, const std::string& algoName, int row, int col) {
    std::cout << "\033[" << row << ";" << col << "H\033[1m" << algoName << " Result\033[0m: Score=" << res.score;
}

void Renderer::drawDotPlot(const Sequence& s1, const Sequence& s2,
                 const std::vector<std::pair<int,int>>& alignPath,
                 int startRow, int startCol) {
    AppState state;
    state.mode = AppMode::DOTPLOT;
    state.seq1 = s1;
    state.seq2 = s2;
    state.dotPlotShowOverlay = true;
    state.tracebackPaths.push_back(alignPath);
    render(state);
}
