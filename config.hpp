#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include "scoring.hpp"

class Config {
public:
    static bool save(const ScoringConfig& cfg, const std::string& filename = "config.txt") {
        std::ofstream f(filename);
        if (!f.is_open()) return false;
        f << "match="      << cfg.match      << "\n";
        f << "mismatch="   << cfg.mismatch   << "\n";
        f << "gap_open="   << cfg.gap_open   << "\n";
        f << "gap_extend=" << cfg.gap_extend << "\n";
        f << "mode="       << (int)cfg.mode  << "\n";
        f.close();
        return true;
    }

    static bool load(ScoringConfig& cfg, const std::string& filename = "config.txt") {
        std::ifstream f(filename);
        if (!f.is_open()) return false;
        std::string line;
        while (std::getline(f, line)) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            int val = std::stoi(line.substr(eq+1));
            if      (key == "match")      cfg.match      = val;
            else if (key == "mismatch")   cfg.mismatch   = val;
            else if (key == "gap_open")   cfg.gap_open   = val;
            else if (key == "gap_extend") cfg.gap_extend = val;
            else if (key == "mode")       cfg.mode       = (ScoringMode)val;
        }
        f.close();
        return true;
    }
};