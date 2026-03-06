#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

using namespace std;

namespace satp::dataset::detail {
    [[nodiscard]] inline uint32_t readU32LE(const uint8_t *bytes) {
        return static_cast<uint32_t>(bytes[0])
               | (static_cast<uint32_t>(bytes[1]) << 8u)
               | (static_cast<uint32_t>(bytes[2]) << 16u)
               | (static_cast<uint32_t>(bytes[3]) << 24u);
    }

    [[nodiscard]] inline uint64_t readU64LE(const uint8_t *bytes) {
        return static_cast<uint64_t>(bytes[0])
               | (static_cast<uint64_t>(bytes[1]) << 8u)
               | (static_cast<uint64_t>(bytes[2]) << 16u)
               | (static_cast<uint64_t>(bytes[3]) << 24u)
               | (static_cast<uint64_t>(bytes[4]) << 32u)
               | (static_cast<uint64_t>(bytes[5]) << 40u)
               | (static_cast<uint64_t>(bytes[6]) << 48u)
               | (static_cast<uint64_t>(bytes[7]) << 56u);
    }

    [[nodiscard]] inline size_t toSizeTChecked(uint64_t value, const string &field) {
        if (value > static_cast<uint64_t>(numeric_limits<size_t>::max())) {
            throw runtime_error("Binary dataset field '" + field + "' is too large for size_t");
        }
        return static_cast<size_t>(value);
    }

    [[nodiscard]] inline streamoff toStreamoffChecked(uint64_t value, const string &field) {
        if (value > static_cast<uint64_t>(numeric_limits<streamoff>::max())) {
            throw runtime_error("Binary dataset field '" + field + "' is too large for streamoff");
        }
        return static_cast<streamoff>(value);
    }
} // namespace satp::dataset::detail

