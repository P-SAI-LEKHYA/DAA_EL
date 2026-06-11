#include <iostream>
#include <string>
#include <limits>
#include "sequence.hpp"
#include "scoring.hpp"
#include "dp_matrix.hpp"
#include "nw.hpp"
#include "sw.hpp"
#include "affine.hpp"
#include "renderer.hpp"
#include "confidence.hpp"
#include "race_mode.hpp"
#include "exporter.hpp"
#include "config.hpp"

void printMenu() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║     DNA SEQUENCE ALIGNMENT TOOL      ║\n";
    std::cout << "╠══════════════════════════════════════╣\n";
    std::cout << "║  1. Needleman-Wunsch (Global)        ║\n";
    std::cout << "║  2. Smith-Waterman (Local)           ║\n";
    std::cout << "║  3. Affine Gap Alignment             ║\n";
    std::cout << "║  4. Algorithm Race Mode              ║\n";
    std::cout << "║  5. Dot Plot                         ║\n";
    std::cout << "║  6. Configure Scoring                ║\n";
    std::cout << "║  7. Export Last Result               ║\n";
    std::cout << "║  8. Exit                             ║\n";
    std::cout << "╚══════════════════════════════════════╝\n";
    std::cout << "Choice: ";
}

ScoringConfig configureScoring() {
    ScoringConfig cfg;
    std::cout << "\nScoring Presets:\n";
    std::cout << "  1. Default (match=2, mismatch=-1, gap=-2)\n";
    std::cout << "  2. Strict  (match=5, mismatch=-4, gap=-6)\n";
    std::cout << "  3. BLOSUM62\n";
    std::cout << "  4. PAM250\n";
    std::cout << "  5. Custom\n";
    std::cout << "Choice: ";
    int c; std::cin >> c;

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
    ScoringConfig cfg;
    Config::load(cfg); // load saved config if exists

    Sequence seq1, seq2;
    AlignmentResult lastResult;
    std::string lastAlgo;
    Renderer renderer;

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

    // length guard
    bool animationMode = (seq1.length() <= 20 && seq2.length() <= 20);
    if (!animationMode) {
        std::cout << "\nNote: Sequences longer than 20 chars — "
                     "summary mode only (no animation).\n";
    }

    int choice = 0;
    while (choice != 8) {
        printMenu();
        std::cin >> choice;

        if (choice == 1) {
            NeedlemanWunsch nw(seq1, seq2, cfg);
            if (animationMode) {
                renderer.clearScreen();
                renderer.hideCursor();
                nw.onStep = [&](int i, int j, int score, int d, int u, int l) {
                    renderer.drawMatrix(nw.matrix, seq1, seq2, {}, i, j, false, 2, 2);
                    renderer.drawSidePanel(2, (seq2.length()+3)*5+5, d, u, l, score);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                };
            }
            nw.fill();
            ConfidenceAnalyzer::compute(nw.matrix, seq1, seq2, cfg, false);
            lastResult = nw.traceback();
            lastAlgo = "Needleman-Wunsch";

            renderer.clearScreen();
            renderer.drawMatrix(nw.matrix, seq1, seq2, lastResult.tracebackPath, -1, -1, false, 2, 2);
            renderer.drawAlignmentResult(lastResult, lastAlgo, seq1.length()+5, 2);

            std::cout << "\nPress C to view confidence heatmap, any other key to continue: ";
            char k; std::cin >> k;
            if (k == 'c' || k == 'C') {
                renderer.clearScreen();
                renderer.drawMatrix(nw.matrix, seq1, seq2, {}, -1, -1, true, 2, 2);
                std::cout << "\n\nBlue = high confidence | Orange = moderate | Grey = uncertain\n";
                std::cout << "Press Enter to continue...";
                std::cin.ignore(); std::cin.get();
            }
            renderer.showCursor();

        } else if (choice == 2) {
            SmithWaterman sw(seq1, seq2, cfg);
            if (animationMode) {
                renderer.clearScreen();
                renderer.hideCursor();
                sw.onStep = [&](int i, int j, int score, int d, int u, int l) {
                    renderer.drawMatrix(sw.matrix, seq1, seq2, {}, i, j, false, 2, 2);
                    renderer.drawSidePanel(2, (seq2.length()+3)*5+5, d, u, l, score);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                };
            }
            sw.fill();
            ConfidenceAnalyzer::compute(sw.matrix, seq1, seq2, cfg, true);
            lastResult = sw.traceback();
            lastAlgo = "Smith-Waterman";

            renderer.clearScreen();
            renderer.drawMatrix(sw.matrix, seq1, seq2, lastResult.tracebackPath, -1, -1, false, 2, 2);
            renderer.drawAlignmentResult(lastResult, lastAlgo, seq1.length()+5, 2);

            std::cout << "\nPress C for confidence heatmap, any other key to continue: ";
            char k; std::cin >> k;
            if (k == 'c' || k == 'C') {
                renderer.clearScreen();
                renderer.drawMatrix(sw.matrix, seq1, seq2, {}, -1, -1, true, 2, 2);
                std::cout << "\n\nBlue = high confidence | Orange = moderate | Grey = uncertain\n";
                std::cout << "Press Enter to continue...";
                std::cin.ignore(); std::cin.get();
            }
            renderer.showCursor();

        } else if (choice == 3) {
            AffineAlignment aff(seq1, seq2, cfg);
            aff.fill();
            AffineResult res = aff.traceback();
            std::cout << "\n=== Affine Gap Alignment ===\n";
            std::cout << "Seq1: " << res.aligned1 << "\n";
            std::cout << "      " << res.matchLine << "\n";
            std::cout << "Seq2: " << res.aligned2 << "\n";
            std::cout << "Score: " << res.score
                      << "  Matches: " << res.matches
                      << "  Mismatches: " << res.mismatches
                      << "  Gaps: " << res.gaps
                      << "  Identity: " << res.identity << "%\n";

        } else if (choice == 4) {
            RaceMode race(seq1, seq2, cfg);
            race.run();
            std::cout << "\nPress Enter to continue...";
            std::cin.ignore(); std::cin.get();

        } else if (choice == 5) {
            NeedlemanWunsch nw(seq1, seq2, cfg);
            nw.fill();
            AlignmentResult res = nw.traceback();
            renderer.clearScreen();
            renderer.drawDotPlot(seq1, seq2, res.tracebackPath, 2, 2);
            std::cout << "\n\nDot = match position  |  * = on alignment path\n";
            std::cout << "Press Enter to continue...";
            std::cin.ignore(); std::cin.get();

        } else if (choice == 6) {
            cfg = configureScoring();
            std::cout << "Scoring config saved.\n";

        } else if (choice == 7) {
            if (lastAlgo.empty()) {
                std::cout << "No result yet. Run an alignment first.\n";
            } else {
                bool ok = Exporter::save(lastResult, lastAlgo, seq1.name, seq2.name);
                std::cout << (ok ? "Saved to alignment_result.txt\n" : "Save failed.\n");
            }
        }
    }

    return 0;
}