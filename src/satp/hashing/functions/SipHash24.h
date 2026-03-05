#pragma once

#include <cstdint>

#include "satp/hashing/HashFunction.h"

using namespace std;

namespace satp::hashing::functions {
    class SipHash24 final : public HashFunction {
    public:
        explicit SipHash24(const uint64_t k0 = 0x0706050403020100ULL,
                           const uint64_t k1 = 0x0f0e0d0c0b0a0908ULL)
            : k0_(k0), k1_(k1) {
        }

        [[nodiscard]] uint64_t hash64(uint64_t value) const override;

        [[nodiscard]] const char *name() const override {
            return "siphash24";
        }

    private:
        uint64_t k0_;
        uint64_t k1_;
    };
} // namespace satp::hashing::functions

