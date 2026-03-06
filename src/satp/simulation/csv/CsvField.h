#pragma once

#include <string>
#include <string_view>

using namespace std;

namespace satp::evaluation::csv {
    [[nodiscard]] inline string escapeCsvField(const string_view value) {
        const bool needsQuotes = value.find_first_of(",\"\n\r") != string_view::npos;
        if (!needsQuotes) return string(value);

        string out;
        out.reserve(value.size() + 2u);
        out.push_back('"');
        for (const char c : value) {
            if (c == '"') {
                out += "\"\"";
            } else {
                out.push_back(c);
            }
        }
        out.push_back('"');
        return out;
    }
} // namespace satp::evaluation::csv
