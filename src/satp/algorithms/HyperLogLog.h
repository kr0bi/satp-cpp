#pragma once

#include <vector>
#include <bit>
#include <cstdint>
#include <limits>

#include "Algorithm.h"

using namespace std;

namespace satp::algorithms {
    class HyperLogLog final : public Algorithm {
    public:
        // Paper-strict mode (Flajolet et al. 2007 practical setting):
        // - k in [4,16], m = 2^k registers
        // - 32-bit hash domain (L = 32)
        explicit HyperLogLog(uint32_t K, uint32_t L);

        void process(uint32_t id) override;

        uint64_t count() override;

        void reset() override;

        string getName() override;

        void merge(const Algorithm &other) override;

        void merge(const HyperLogLog &other);

    private:
        uint32_t k;
        uint32_t numberOfBuckets; // nel paper coincide con m
        uint32_t lengthOfBitMap;
        // Logical register values fit in <=5 bits in paper settings; we store in uint8_t.
        vector<uint8_t> bitmap;
        double alphaM;
        double sumInversePowers; // \sum_j 2^{-M[j]}
        uint32_t zeroRegisters;

        static constexpr double ALPHA_16 = 0.673;
        static constexpr double ALPHA_32 = 0.697;
        static constexpr double ALPHA_64 = 0.709;
    };
} // namespace satp::algorithms
