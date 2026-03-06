#include <array>
#include <cstdint>
#include <optional>

#include "catch2/catch_test_macros.hpp"

#include "satp/hashing/HashFactory.h"
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

TEST_CASE("Factory seeded hashers use dataset seed", "[hashing][factory]") {
    constexpr array<uint64_t, 4> inputs{
        1ULL,
        42ULL,
        0x0123456789abcdefULL,
        0xffffffffffffffffULL,
    };

    const auto xxA = satp::hashing::getHashFunctionBy("xxhash64", 111u);
    const auto xxB = satp::hashing::getHashFunctionBy("xxhash64", 222u);
    bool xxDiff = false;
    for (const uint64_t input : inputs) {
        if (xxA->hash64(input) != xxB->hash64(input)) {
            xxDiff = true;
            break;
        }
    }
    REQUIRE(xxDiff);

    const auto murmurA = satp::hashing::getHashFunctionBy("murmurhash3", 111u);
    const auto murmurB = satp::hashing::getHashFunctionBy("murmurhash3", 222u);
    bool murmurDiff = false;
    for (const uint64_t input : inputs) {
        if (murmurA->hash64(input) != murmurB->hash64(input)) {
            murmurDiff = true;
            break;
        }
    }
    REQUIRE(murmurDiff);
}

TEST_CASE("Factory enforces optional name/seed contract", "[hashing][factory]") {
    REQUIRE_NOTHROW(satp::hashing::getHashFunctionBy());
    REQUIRE_NOTHROW(satp::hashing::getHashFunctionBy("", 123u));
    REQUIRE_THROWS_AS(satp::hashing::getHashFunctionBy("splitmix64"), invalid_argument);
    REQUIRE_THROWS_AS(satp::hashing::getHashFunctionBy(nullopt, 123u), invalid_argument);
}
