#pragma once

#include <cstdint>

#include "satp/hashing/HashFunction.h"

using namespace std;

namespace satp::hashing::functions {
    class SplitMix64 final : public HashFunction {
    public:
        [[nodiscard]] uint64_t hash64(uint64_t value) const override;

        [[nodiscard]] const char *name() const override {
            return "splitmix64";
        }
    };
} // namespace satp::hashing::functions

