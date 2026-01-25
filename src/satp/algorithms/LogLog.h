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
        // k = number of register index bits (m = 2^k registers)
        explicit LogLog(uint32_t K, uint32_t L);

        void process(uint32_t id) override;

        uint64_t count() override;

        void reset() override;

        string getName() override;

    private:
        uint32_t k;
        uint32_t numberOfBuckets;
        uint32_t lengthOfBitMap;
        vector<uint32_t> bitmap;

        static constexpr double ALPHA_INF = 0.39701;
    };
} // namespace satp::algorithms
