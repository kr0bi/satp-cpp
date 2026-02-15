#pragma once
#include <cstdint>

namespace util::hashing {
    // 64-bit hash used as the single source of randomness.
    [[nodiscard]]
    inline constexpr std::uint64_t splitmix64(std::uint64_t x) {
        x += 0x9E3779B97F4A7C15ULL;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
        return x ^ (x >> 31);
    }

    // 32-bit hash obtained by truncating the 64-bit hash (use upper bits).
    [[nodiscard]]
    inline constexpr std::uint32_t hash32_from_64(std::uint64_t h) noexcept {
        return static_cast<std::uint32_t>(h >> 32);
    }
} // namespace util::hashing
