#pragma once
#include <string>
#include <stdexcept>
#include <algorithm>

struct Sequence {
    std::string name;
    std::string data;

    Sequence() = default;
    Sequence(const std::string& name, const std::string& data)
        : name(name), data(data) {
        validate();
    }

    void validate() const {
        for (char c : data) {
            char u = toupper(c);
            if (u != 'A' && u != 'T' && u != 'G' && u != 'C') {
                throw std::invalid_argument(
                    std::string("Invalid character in sequence: ") + c
                );
            }
        }
    }

    void normalize() {
        for (char& c : data) c = toupper(c);
    }

    int length() const { return (int)data.size(); }

    char operator[](int i) const { return data[i]; }
};