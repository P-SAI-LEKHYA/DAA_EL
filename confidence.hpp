#pragma once
#include "dp_matrix.hpp"
#include "sequence.hpp"
#include "scoring.hpp"
#include <vector>
#include <climits>

// Computes confidence for every cell:
// confidence = how far the best candidate is above the second best
// High confidence = one clear choice was made
// Low confidence  = multiple paths were nearly equally good

class ConfidenceAnalyzer {
public:
    static void compute(DPMatrix& mat, const Sequence& seq1,
                        const Sequence& seq2, const ScoringConfig& cfg,
                        bool isSW = false) {
        int m = mat.rows - 1;
        int n = mat.cols - 1;

        for (int i = 1; i <= m; i++) {
            for (int j = 1; j <= n; j++) {
                int diag = mat.at(i-1,j-1).score + cfg.getScore(seq1[i-1], seq2[j-1]);
                int up   = mat.at(i-1,j).score   + cfg.gap_open;
                int left = mat.at(i,j-1).score   + cfg.gap_open;

                std::vector<int> candidates = {diag, up, left};
                if (isSW) candidates.push_back(0);

                int best = *std::max_element(candidates.begin(), candidates.end());

                // find second best (different from best index)
                int secondBest = INT_MIN;
                for (int v : candidates)
                    if (v != best && v > secondBest) secondBest = v;
                if (secondBest == INT_MIN) secondBest = best - 10;

                float diff = (float)(best - secondBest);
                // normalize: 0 = completely uncertain, 1 = very certain
                mat.at(i,j).confidence = std::min(1.0f, std::max(0.0f, diff / 6.0f));
            }
        }
    }
};