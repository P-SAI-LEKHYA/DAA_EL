#pragma once
#include <string>
#include <vector>
#include <iostream>
#include "dp_matrix.hpp"
#include "sequence.hpp"
#include "scoring.hpp"
#include "nw.hpp" // for AlignmentResult

enum class AppMode {
    MAIN,
    MUTATION,
    RACE,
    DOTPLOT
};

struct RendererConfig {
    std::string matchColor = "\033[34m";          // ANSI 34 (blue)
    std::string mismatchColor = "\033[38;5;208m";     // ANSI 208 (orange)
    std::string gapColor = "\033[37m";            // ANSI 37 (white)
    
    std::string tracebackColor1 = "\033[1;36m";   // ANSI 36 bold (cyan)
    std::string tracebackColor2 = "\033[1;35m";   // ANSI 35 bold (magenta)
    std::string tracebackColor3 = "\033[1;33m";   // ANSI 33 bold (yellow)

    std::string heatmapLow = "\033[33m";          // ANSI 33 (yellow)
    std::string heatmapMid = "\033[38;5;208m";    // ANSI 208 (orange)
    std::string heatmapHigh = "\033[32m";         // ANSI 32 (green)
};

struct AppState {
    AppMode mode = AppMode::MAIN;
    Sequence seq1;
    Sequence seq2;
    DPMatrix mainMatrix;
    
    // Hover details
    int hoverRow = 0;
    int hoverCol = 0;
    
    // Side panel toggle
    bool sidePanelOpen = true;
    
    // Heatmap mode toggle
    bool heatmapOverlay = false;
    
    // Traceback paths
    std::vector<std::vector<std::pair<int, int>>> tracebackPaths; // multiple paths
    
    // Score configuration
    ScoringConfig scoringConfig;
    
    // Main result alignment stats
    AlignmentResult mainResult;
    std::string currentAlgorithmName = "Needleman-Wunsch";
    
    // Mutation mode details
    int mutationCursorIdx = 0; // cursor position in sequence
    bool editingSeq2 = false; // editing seq1 (vertical, rows) or seq2 (horizontal, cols)
    std::string mutationInputBuffer;
    int flashFramesLeft = 0; // inverted flash animation frame count
    std::vector<std::pair<int, int>> affectedCells; // list of changed cells
    int deltaScore = 0;
    bool showDeltaScore = false;
    
    // Dot plot details
    bool dotPlotShowOverlay = false; // false = raw dot plot, true = alignment overlay
    
    // Race Mode details
    bool raceModeActive = false;
    bool raceCompleted = false;
    DPMatrix nwMatrix;
    DPMatrix swMatrix;
    DPMatrix greedyMatrix;
    int nwCells = 0;
    int swCells = 0;
    int greedyCells = 0;
    long long nwTimeUs = 0;
    long long swTimeUs = 0;
    long long greedyTimeUs = 0;
    
    // Summary
    std::string scoreWinner;
    std::string speedWinner;
};

class Renderer {
private:
    RendererConfig config;
    int cellWidth = 5;

    // Helper drawing functions
    void drawMainView(const AppState& state);
    void drawRaceView(const AppState& state);
    void drawDotPlotView(const AppState& state);
    void drawStatusBar(const AppState& state);

    std::string formatCell(int score, Direction dir, bool isUncomputed) const;
    std::string getCellColor(const AppState& state, int r, int c) const;

public:
    Renderer() = default;
    
    void clearScreen() const;
    void hideCursor() const;
    void showCursor() const;
    
    // Main single render call per frame
    void render(const AppState& state);
    
    // Legacy support (optional helper wrappers)
    void drawMatrix(
        const DPMatrix& mat,
        const Sequence& seq1,
        const Sequence& seq2,
        const std::vector<std::pair<int,int>>& traceback = {},
        int activeRow = -1, int activeCol = -1,
        bool showConfidence = false,
        int startRow = 1, int startCol = 1
    );
    void drawSidePanel(int row, int col, int diagVal, int upVal, int leftVal, int chosen);
    void drawAlignmentResult(const AlignmentResult& res, const std::string& algoName, int row, int col);
    void drawDotPlot(const Sequence& s1, const Sequence& s2,
                     const std::vector<std::pair<int,int>>& alignPath,
                     int startRow, int startCol);
};