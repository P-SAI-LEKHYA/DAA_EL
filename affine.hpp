#pragma once
#include "sequence.hpp"
#include "scoring.hpp"
#include "dp_matrix.hpp"
#include <vector>
#include <climits>

// Affine gap uses 3 matrices:
// M[i][j]  = best score ending with a match/mismatch
// Ix[i][j] = best score ending with gap in seq2 (vertical gap)
// Iy[i][j] = best score ending with gap in seq1 (horizontal gap)

struct AffineResult {
    std::string aligned1;
    std::string aligned2;
    std::string matchLine;
    int score = 0;
    int matches = 0;
    int mismatches = 0;
    int gaps = 0;
    float identity = 0.0f;
};

class AffineAlignment {
public:
    const Sequence& seq1;
    const Sequence& seq2;
    const ScoringConfig& config;

    int m, n;
    std::vector<std::vector<int>> M, Ix, Iy;

    static constexpr int NEG_INF = INT_MIN / 2;

    AffineAlignment(const Sequence& s1, const Sequence& s2, const ScoringConfig& cfg)
        : seq1(s1), seq2(s2), config(cfg),
          m(s1.length()), n(s2.length()) {
        M.assign(m+1,  std::vector<int>(n+1, NEG_INF));
        Ix.assign(m+1, std::vector<int>(n+1, NEG_INF));
        Iy.assign(m+1, std::vector<int>(n+1, NEG_INF));
    }

    void fill() {
        M[0][0]  = 0;
        Ix[0][0] = NEG_INF;
        Iy[0][0] = NEG_INF;

        for (int i = 1; i <= m; i++) {
            M[i][0]  = NEG_INF;
            Ix[i][0] = config.gap_open + (i-1) * config.gap_extend;
            Iy[i][0] = NEG_INF;
        }
        for (int j = 1; j <= n; j++) {
            M[0][j]  = NEG_INF;
            Ix[0][j] = NEG_INF;
            Iy[0][j] = config.gap_open + (j-1) * config.gap_extend;
        }

        for (int i = 1; i <= m; i++) {
            for (int j = 1; j <= n; j++) {
                int s = config.getScore(seq1[i-1], seq2[j-1]);

                M[i][j] = std::max({
                    M[i-1][j-1]  + s,
                    Ix[i-1][j-1] + s,
                    Iy[i-1][j-1] + s
                });

                Ix[i][j] = std::max(
                    M[i-1][j]  + config.gap_open,
                    Ix[i-1][j] + config.gap_extend
                );

                Iy[i][j] = std::max(
                    M[i][j-1]  + config.gap_open,
                    Iy[i][j-1] + config.gap_extend
                );
            }
        }
    }

    AffineResult traceback() {
        AffineResult result;
        result.score = std::max({M[m][n], Ix[m][n], Iy[m][n]});

        // which matrix are we in?
        enum State { IN_M, IN_IX, IN_IY };
        State state;
        if (result.score == M[m][n])       state = IN_M;
        else if (result.score == Ix[m][n]) state = IN_IX;
        else                               state = IN_IY;

        int i = m, j = n;
        while (i > 0 || j > 0) {
            if (state == IN_M) {
                result.aligned1 = seq1[i-1] + result.aligned1;
                result.aligned2 = seq2[j-1] + result.aligned2;
                if (seq1[i-1] == seq2[j-1]) {
                    result.matchLine = '|' + result.matchLine;
                    result.matches++;
                } else {
                    result.matchLine = '.' + result.matchLine;
                    result.mismatches++;
                }
                int s = config.getScore(seq1[i-1], seq2[j-1]);
                if      (M[i-1][j-1]  + s == M[i][j]) state = IN_M;
                else if (Ix[i-1][j-1] + s == M[i][j]) state = IN_IX;
                else                                    state = IN_IY;
                i--; j--;
            } else if (state == IN_IX) {
                result.aligned1 = seq1[i-1] + result.aligned1;
                result.aligned2 = '-' + result.aligned2;
                result.matchLine = ' ' + result.matchLine;
                result.gaps++;
                if (M[i-1][j] + config.gap_open == Ix[i][j]) state = IN_M;
                else                                           state = IN_IX;
                i--;
            } else {
                result.aligned1 = '-' + result.aligned1;
                result.aligned2 = seq2[j-1] + result.aligned2;
                result.matchLine = ' ' + result.matchLine;
                result.gaps++;
                if (M[i][j-1] + config.gap_open == Iy[i][j]) state = IN_M;
                else                                           state = IN_IY;
                j--;
            }
        }

        int len = result.aligned1.length();
        result.identity = len > 0 ? (float)result.matches / len * 100.0f : 0.0f;
        return result;
    }
};