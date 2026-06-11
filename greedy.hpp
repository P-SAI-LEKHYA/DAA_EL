#pragma once
#include "dp_matrix.hpp"
#include "scoring.hpp"
#include "sequence.hpp"
#include "nw.hpp" // for AlignmentResult
#include <functional>
#include <climits>
#include <vector>
#include <algorithm>

class GreedyAlignment {
public:
    DPMatrix matrix;
    const Sequence& seq1;
    const Sequence& seq2;
    const ScoringConfig& config;

    using StepCallback = std::function<void(int,int,int,int,int,int)>;
    StepCallback onStep;

    std::vector<std::pair<int, int>> greedyPath;

    GreedyAlignment(const Sequence& s1, const Sequence& s2, const ScoringConfig& cfg)
        : seq1(s1), seq2(s2), config(cfg),
          matrix(s1.length()+1, s2.length()+1) {}

    void fill() {
        int m = seq1.length();
        int n = seq2.length();

        // Initialize all matrix cells as uncomputed (-9999)
        for (int i = 0; i <= m; i++) {
            for (int j = 0; j <= n; j++) {
                matrix.at(i, j).score = -9999;
                matrix.at(i, j).dir = Direction::NONE;
                matrix.at(i, j).confidence = 0.0f;
            }
        }

        int i = 0;
        int j = 0;
        matrix.at(0, 0).score = 0;
        matrix.at(0, 0).dir = Direction::NONE;
        greedyPath.clear();
        greedyPath.push_back({0, 0});

        if (onStep) onStep(0, 0, 0, 0, 0, 0);

        while (i < m || j < n) {
            int diag = -9999, up = -9999, left = -9999;

            if (i < m && j < n) {
                diag = config.getScore(seq1[i], seq2[j]);
            }
            if (i < m) {
                up = config.gap_open;
            }
            if (j < n) {
                left = config.gap_open;
            }

            int best = std::max({diag, up, left});

            int ni = i, nj = j;
            Direction d = Direction::NONE;

            // Greedily choose the best local direction
            // Priority: Diagonal (match/mismatch) > Up (gap in seq2) > Left (gap in seq1)
            if (i < m && j < n && best == diag) {
                ni = i + 1;
                nj = j + 1;
                d = Direction::DIAG;
            } else if (i < m && best == up) {
                ni = i + 1;
                d = Direction::UP;
            } else if (j < n) {
                nj = j + 1;
                d = Direction::LEFT;
            }

            int stepScore = (d == Direction::DIAG) ? diag : ((d == Direction::UP) ? up : left);
            matrix.at(ni, nj).score = matrix.at(i, j).score + stepScore;
            matrix.at(ni, nj).dir = d;
            // For greedy, confidence is high since it is a single deterministic path
            matrix.at(ni, nj).confidence = 1.0f; 

            i = ni;
            j = nj;
            greedyPath.push_back({i, j});

            if (onStep) {
                onStep(i, j, matrix.at(i, j).score, diag, up, left);
            }
        }
    }

    AlignmentResult traceback() {
        AlignmentResult result;
        int m = seq1.length();
        int n = seq2.length();
        result.score = matrix.at(m, n).score;
        
        // Traceback path is exactly the greedy path
        result.tracebackPath = greedyPath;

        int i = m;
        int j = n;
        while (i > 0 || j > 0) {
            Direction d = matrix.at(i, j).dir;
            if (d == Direction::DIAG) {
                result.aligned1 = seq1[i-1] + result.aligned1;
                result.aligned2 = seq2[j-1] + result.aligned2;
                if (seq1[i-1] == seq2[j-1]) {
                    result.matchLine = '|' + result.matchLine;
                    result.matches++;
                } else {
                    result.matchLine = '.' + result.matchLine;
                    result.mismatches++;
                }
                i--; j--;
            } else if (d == Direction::UP) {
                result.aligned1 = seq1[i-1] + result.aligned1;
                result.aligned2 = '-' + result.aligned2;
                result.matchLine = ' ' + result.matchLine;
                result.gaps++;
                i--;
            } else {
                result.aligned1 = '-' + result.aligned1;
                result.aligned2 = seq2[j-1] + result.aligned2;
                result.matchLine = ' ' + result.matchLine;
                result.gaps++;
                j--;
            }
        }

        int len = result.aligned1.length();
        result.identity = len > 0 ? (float)result.matches / len * 100.0f : 0.0f;
        return result;
    }
};
