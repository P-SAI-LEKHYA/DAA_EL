#pragma once
#include <fstream>
#include <string>
#include <ctime>
#include "nw.hpp"

class Exporter {
public:
    static bool save(const AlignmentResult& res,
                     const std::string& algoName,
                     const std::string& seq1name,
                     const std::string& seq2name,
                     const std::string& filename = "alignment_result.txt") {
        std::ofstream f(filename);
        if (!f.is_open()) return false;

        // timestamp
        time_t now = time(nullptr);
        f << "# DNA Alignment Result\n";
        f << "# Generated: " << ctime(&now);
        f << "# Algorithm: " << algoName << "\n\n";

        f << "Sequence 1 (" << seq1name << "):\n";
        f << res.aligned1 << "\n";
        f << std::string(res.aligned1.size(), '-') << "\n";
        f << res.matchLine << "\n";
        f << std::string(res.aligned2.size(), '-') << "\n";
        f << "Sequence 2 (" << seq2name << "):\n";
        f << res.aligned2 << "\n\n";

        f << "=== Statistics ===\n";
        f << "Alignment Score : " << res.score      << "\n";
        f << "Matches         : " << res.matches     << "\n";
        f << "Mismatches      : " << res.mismatches  << "\n";
        f << "Gaps            : " << res.gaps        << "\n";
        f << "Identity        : " << res.identity    << "%\n";

        f.close();
        return true;
    }
};