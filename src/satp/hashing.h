// hashing.hpp ---------------------------------------------------------------
#pragma once
#include <cstdint>

using namespace std;

namespace util::hashing {
    [[nodiscard]]
    inline constexpr uint32_t uniform_hash(uint32_t x,
                                           uint32_t lengthBitMap) noexcept {
        x += 0x9E3779B9u;
        x = (x ^ (x >> 16)) * 0x85EBCA6Bu;
        x = (x ^ (x >> 13)) * 0xC2B2AE35u;
        x ^= (x >> 16);
        if (lengthBitMap == 0) return 0u;
        if (lengthBitMap >= 32) return x;
        const uint32_t mask = (1u << lengthBitMap) - 1u;
        return x & mask;
    }
} // namespace util::hashing
