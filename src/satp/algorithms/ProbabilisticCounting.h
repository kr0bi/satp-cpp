#pragma once

#include <vector>
#include <bit>
#include <cstdint>
#include <limits>

#include "Algorithm.h"

using namespace std;

namespace satp::algorithms {
    class ProbabilisticCounting final : public Algorithm {
    public:
        explicit ProbabilisticCounting(uint32_t L);

        void process(uint32_t id) override;

        uint64_t count() override;

        void reset() override;

    private:
        uint32_t lengthBitMap;
        uint32_t bitmap;

        static constexpr double INV_PHI = 0.77351;
    };
} // namespace satp::algorithms
