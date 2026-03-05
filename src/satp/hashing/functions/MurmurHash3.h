#pragma once

#include <cstdint>

#include "satp/hashing/HashFunction.h"

using namespace std;

namespace satp::hashing::functions {
    class MurmurHash3 final : public HashFunction {
    public:
        explicit MurmurHash3(const uint32_t seed = 0u)
            : seed_(seed) {
        }

        [[nodiscard]] uint64_t hash64(uint64_t value) const override;

        [[nodiscard]] const char *name() const override {
            return "murmurhash3";
        }

    private:
        uint32_t seed_;
    };
} // namespace satp::hashing::functions

