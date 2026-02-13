#pragma once

#include <vector>
#include <bit>
#include <cstdint>
#include <limits>

#include "Algorithm.h"

using namespace std;

namespace satp::algorithms {
    class LogLog final : public Algorithm {
    public:
        // Paper-strict mode (Durand-Flajolet 2003 / HLL 2007 practical range):
        // - k in [4,16], m = 2^k registers
        // - 32-bit hash domain (L = 32)
        explicit LogLog(uint32_t K, uint32_t L);

        void process(uint32_t id) override;

        uint64_t count() override;

        void reset() override;

        string getName() override;

    private:
        uint32_t k;
        uint32_t numberOfBuckets;
        uint32_t lengthOfBitMap;
        // Logical register values fit in <=5 bits in paper settings; we store in uint8_t.
        vector<uint8_t> bitmap;
        double sumRegisters; // \sum_j M[j]

        static constexpr double ALPHA_INF = 0.39701;
    };
} // namespace satp::algorithms
