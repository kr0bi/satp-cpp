#include "satp/hashing/functions/SplitMix64.h"

using namespace std;

namespace satp::hashing::functions {
    uint64_t SplitMix64::hash64(uint64_t value) const {
        value += 0x9E3779B97F4A7C15ULL;
        value = (value ^ (value >> 30u)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27u)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31u);
    }
} // namespace satp::hashing::functions

