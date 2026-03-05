#include "satp/hashing/functions/SipHash24.h"

#include <bit>

using namespace std;

namespace satp::hashing::functions {
    namespace {
        void sipRound(uint64_t &v0,
                      uint64_t &v1,
                      uint64_t &v2,
                      uint64_t &v3) {
            v0 += v1;
            v1 = rotl(v1, 13);
            v1 ^= v0;
            v0 = rotl(v0, 32);

            v2 += v3;
            v3 = rotl(v3, 16);
            v3 ^= v2;

            v0 += v3;
            v3 = rotl(v3, 21);
            v3 ^= v0;

            v2 += v1;
            v1 = rotl(v1, 17);
            v1 ^= v2;
            v2 = rotl(v2, 32);
        }
    } // namespace

    uint64_t SipHash24::hash64(const uint64_t value) const {
        uint64_t v0 = 0x736f6d6570736575ULL ^ k0_;
        uint64_t v1 = 0x646f72616e646f6dULL ^ k1_;
        uint64_t v2 = 0x6c7967656e657261ULL ^ k0_;
        uint64_t v3 = 0x7465646279746573ULL ^ k1_;

        v3 ^= value;
        sipRound(v0, v1, v2, v3);
        sipRound(v0, v1, v2, v3);
        v0 ^= value;

        constexpr uint64_t lenBlock = (8ULL << 56u); // len = 8, no tail bytes
        v3 ^= lenBlock;
        sipRound(v0, v1, v2, v3);
        sipRound(v0, v1, v2, v3);
        v0 ^= lenBlock;

        v2 ^= 0xffULL;
        sipRound(v0, v1, v2, v3);
        sipRound(v0, v1, v2, v3);
        sipRound(v0, v1, v2, v3);
        sipRound(v0, v1, v2, v3);

        return v0 ^ v1 ^ v2 ^ v3;
    }
} // namespace satp::hashing::functions

