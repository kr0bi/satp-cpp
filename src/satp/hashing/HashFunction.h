#pragma once

#include <cstdint>

using namespace std;

namespace satp::hashing {
    class HashFunction {
    public:
        virtual ~HashFunction() = default;

        [[nodiscard]] virtual uint64_t hash64(uint64_t value) const = 0;

        // Default 32-bit projection for 64-bit native hashers: keep upper 32 bits.
        [[nodiscard]] virtual uint32_t hash32(const uint64_t value) const {
            return static_cast<uint32_t>(hash64(value) >> 32u);
        }

        [[nodiscard]] virtual const char *name() const = 0;
    };
} // namespace satp::hashing
