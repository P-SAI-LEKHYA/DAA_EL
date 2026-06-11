#pragma once
#include <string>
#include <map>

enum class ScoringMode { FLAT, BLOSUM62, PAM250 };

struct ScoringConfig {
    int match       =  2;
    int mismatch    = -1;
    int gap_open    = -2;
    int gap_extend  = -1;
    ScoringMode mode = ScoringMode::FLAT;

    // BLOSUM62 hardcoded (A,T,G,C subset)
    // Full 4x4 for DNA context
    static const int BLOSUM62[4][4];
    static const int PAM250[4][4];

    // index: A=0, T=1, G=2, C=3
    static int charToIdx(char c) {
        switch(toupper(c)) {
            case 'A': return 0;
            case 'T': return 1;
            case 'G': return 2;
            case 'C': return 3;
            default:  return -1;
        }
    }

    int getScore(char a, char b) const {
        if (mode == ScoringMode::FLAT) {
            return (toupper(a) == toupper(b)) ? match : mismatch;
        }
        int i = charToIdx(a);
        int j = charToIdx(b);
        if (i < 0 || j < 0) return mismatch;
        if (mode == ScoringMode::BLOSUM62) return BLOSUM62[i][j];
        if (mode == ScoringMode::PAM250)   return PAM250[i][j];
        return mismatch;
    }
};

// DNA-specific BLOSUM62 subset
inline const int ScoringConfig::BLOSUM62[4][4] = {
    // A   T   G   C
    {  4, -1, -2, -2 },  // A
    { -1,  5, -2, -1 },  // T
    { -2, -2,  6, -3 },  // G
    { -2, -1, -3,  9 }   // C
};

// DNA-specific PAM250 subset
inline const int ScoringConfig::PAM250[4][4] = {
    // A   T   G   C
    {  2, -1,  0, -1 },  // A
    { -1,  3, -1,  0 },  // T
    {  0, -1,  4, -1 },  // G
    { -1,  0, -1,  5 }   // C
};