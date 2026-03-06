#pragma once

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string_view>

using namespace std;

namespace satp::evaluation::csv {
    inline void writeHeaderIfNeeded(const filesystem::path &csvPath,
                                    ofstream &out,
                                    const string_view header) {
        const bool shouldWriteHeader = !filesystem::exists(csvPath) || filesystem::file_size(csvPath) == 0u;
        if (shouldWriteHeader) {
            out << header << '\n';
        }
    }

    [[nodiscard]] inline ofstream openAppend(const filesystem::path &csvPath,
                                             const string_view header,
                                             const char *errorMessage) {
        ofstream out(csvPath, ios::app);
        if (!out) {
            throw runtime_error(errorMessage);
        }
        out << setprecision(10);
        writeHeaderIfNeeded(csvPath, out, header);
        return out;
    }
} // namespace satp::evaluation::csv
