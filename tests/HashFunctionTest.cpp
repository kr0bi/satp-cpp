#include <array>
#include <cstdint>

#include "catch2/catch_test_macros.hpp"

#include "satp/hashing/functions/MurmurHash3.h"
#include "satp/hashing/functions/SipHash24.h"
#include "satp/hashing/functions/SplitMix64.h"
#include "satp/hashing/functions/XXHash64.h"

using namespace std;

namespace {
    template<typename THasher>
    void assertDeterministicAndProjected32(const THasher &hasher) {
        constexpr array<uint64_t, 6> inputs{
            0ULL,
            1ULL,
            2ULL,
            42ULL,
            0x0123456789abcdefULL,
            0xffffffffffffffffULL,
        };

        for (const uint64_t input : inputs) {
            const uint64_t h64a = hasher.hash64(input);
            const uint64_t h64b = hasher.hash64(input);
            REQUIRE(h64a == h64b);

            const auto projected32 = static_cast<uint32_t>(h64a >> 32u);
            REQUIRE(hasher.hash32(input) == projected32);
        }
    }
} // namespace

TEST_CASE("SplitMix64 deterministic and hash32 projection", "[hashing]") {
    const satp::hashing::functions::SplitMix64 hasher{};
    assertDeterministicAndProjected32(hasher);
}

TEST_CASE("XXHash64 deterministic and hash32 projection", "[hashing]") {
    const satp::hashing::functions::XXHash64 hasher{};
    assertDeterministicAndProjected32(hasher);
}

TEST_CASE("MurmurHash3 deterministic and hash32 projection", "[hashing]") {
    const satp::hashing::functions::MurmurHash3 hasher{};
    assertDeterministicAndProjected32(hasher);
}

TEST_CASE("SipHash24 deterministic and hash32 projection", "[hashing]") {
    const satp::hashing::functions::SipHash24 hasher{};
    assertDeterministicAndProjected32(hasher);
}
