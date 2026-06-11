#pragma once
#include "dp_matrix.hpp"
#include "scoring.hpp"
#include "sequence.hpp"
#include <vector>
#include <string>
#include <functional>

struct AlignmentResult {
    std::string aligned1;
    std::string aligned2;
    std::string matchLine;
    int score = 0;
    int matches = 0;
    int mismatches = 0;
    int gaps = 0;
    float identity = 0.0f;
    std::vector<std::pair<int,int>> tracebackPath;
};

class NeedlemanWunsch {
public:
    DPMatrix matrix;
    const Sequence& seq1;
    const Sequence& seq2;
    const ScoringConfig& config;

    // callback called after each cell fill (for animation)
    // args: row, col, score, candidate_diag, candidate_up, candidate_left
    using StepCallback = std::function<void(int,int,int,int,int,int)>;
    StepCallback onStep;

    NeedlemanWunsch(const Sequence& s1, const Sequence& s2, const ScoringConfig& cfg)
        : seq1(s1), seq2(s2), config(cfg),
          matrix(s1.length()+1, s2.length()+1) {}

    void fill() {
        int m = seq1.length();
        int n = seq2.length();

        // initialize first row and column
        for (int i = 0; i <= m; i++) {
            matrix.at(i,0).score = i * config.gap_open;
            matrix.at(i,0).dir   = Direction::UP;
        }
        for (int j = 0; j <= n; j++) {
            matrix.at(0,j).score = j * config.gap_open;
            matrix.at(0,j).dir   = Direction::LEFT;
        }
        matrix.at(0,0).dir = Direction::NONE;

        for (int i = 1; i <= m; i++) {
            for (int j = 1; j <= n; j++) {
                int diag = matrix.at(i-1,j-1).score + config.getScore(seq1[i-1], seq2[j-1]);
                int up   = matrix.at(i-1,j).score   + config.gap_open;
                int left = matrix.at(i,j-1).score   + config.gap_open;

                int best = std::max({diag, up, left});
                matrix.at(i,j).score = best;

                // direction + confidence
                int secondBest = INT_MIN;
                if (best == diag) {
                    matrix.at(i,j).dir = Direction::DIAG;
                    secondBest = std::max(up, left);
                } else if (best == up) {
                    matrix.at(i,j).dir = Direction::UP;
                    secondBest = std::max(diag, left);
                } else {
                    matrix.at(i,j).dir = Direction::LEFT;
                    secondBest = std::max(diag, up);
                }

                // confidence: how far ahead is the best vs second best
                float diff = (float)(best - secondBest);
                matrix.at(i,j).confidence = std::min(1.0f, diff / 5.0f);

                if (onStep) onStep(i, j, best, diag, up, left);
            }
        }
    }

    AlignmentResult traceback() {
        AlignmentResult result;
        result.score = matrix.at(seq1.length(), seq2.length()).score;

        int i = seq1.length();
        int j = seq2.length();

        while (i > 0 || j > 0) {
            result.tracebackPath.push_back({i, j});
            Direction d = matrix.at(i,j).dir;

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
        result.tracebackPath.push_back({0,0});

        int len = result.aligned1.length();
        result.identity = len > 0 ? (float)result.matches / len * 100.0f : 0.0f;

        return result;
    }
};