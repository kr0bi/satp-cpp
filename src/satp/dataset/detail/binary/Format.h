#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

using namespace std;

namespace satp::dataset::detail {
    constexpr array<char, 8> MAGIC = {'S', 'A', 'T', 'P', 'D', 'B', 'N', '2'};
    constexpr uint32_t VERSION = 2u;
    constexpr uint32_t ENCODING_ZLIB_U32_LE = 1u;
    constexpr uint32_t ENCODING_ZLIB_BITSET_LE = 2u;
    constexpr size_t HEADER_SIZE = 44u;
    constexpr size_t ENTRY_SIZE = 60u;
} // namespace satp::dataset::detail

