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


    [[nodiscard]]
    inline constexpr uint64_t splitmix64(uint64_t x) {
        x += 0x9E3779B97F4A7C15ULL;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
        return x ^ (x >> 31);
    }
} // namespace util::hashing
