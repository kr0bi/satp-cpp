#include "satp/hashing/functions/MurmurHash3.h"

#include <bit>

using namespace std;

namespace satp::hashing::functions {
    namespace {
        [[nodiscard]] uint64_t fmix64(uint64_t value) {
            value ^= value >> 33u;
            value *= 0xff51afd7ed558ccdULL;
            value ^= value >> 33u;
            value *= 0xc4ceb9fe1a85ec53ULL;
            value ^= value >> 33u;
            return value;
        }
    } // namespace

    uint64_t MurmurHash3::hash64(const uint64_t value) const {
        // MurmurHash3_x64_128 reduced to one 64-bit block (len = 8), returns h1.
        constexpr uint64_t C1 = 0x87c37b91114253d5ULL;
        constexpr uint64_t C2 = 0x4cf5ad432745937fULL;

        uint64_t h1 = seed_;
        uint64_t h2 = seed_;

        uint64_t k1 = value;
        k1 *= C1;
        k1 = rotl(k1, 31);
        k1 *= C2;
        h1 ^= k1;

        h1 = rotl(h1, 27);
        h1 += h2;
        h1 = h1 * 5ULL + 0x52dce729ULL;

        constexpr uint64_t LEN = 8ULL;
        h1 ^= LEN;
        h2 ^= LEN;

        h1 += h2;
        h2 += h1;

        h1 = fmix64(h1);
        h2 = fmix64(h2);

        h1 += h2;
        return h1;
    }
} // namespace satp::hashing::functions

