#include "satp/hashing/functions/XXHash64.h"

#include <bit>

using namespace std;

namespace satp::hashing::functions {
    namespace {
        constexpr uint64_t PRIME64_1 = 11400714785074694791ULL;
        constexpr uint64_t PRIME64_2 = 14029467366897019727ULL;
        constexpr uint64_t PRIME64_3 = 1609587929392839161ULL;
        constexpr uint64_t PRIME64_4 = 9650029242287828579ULL;
        constexpr uint64_t PRIME64_5 = 2870177450012600261ULL;
    } // namespace

    uint64_t XXHash64::hash64(const uint64_t value) const {
        // xxHash64 one-shot for exactly 8 bytes.
        uint64_t h64 = seed_ + PRIME64_5 + 8ULL;

        uint64_t k1 = value;
        k1 *= PRIME64_2;
        k1 = rotl(k1, 31);
        k1 *= PRIME64_1;
        h64 ^= k1;

        h64 = rotl(h64, 27);
        h64 = h64 * PRIME64_1 + PRIME64_4;

        h64 ^= h64 >> 33u;
        h64 *= PRIME64_2;
        h64 ^= h64 >> 29u;
        h64 *= PRIME64_3;
        h64 ^= h64 >> 32u;

        return h64;
    }
} // namespace satp::hashing::functions

