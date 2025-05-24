#pragma once

#include <vector>
#include <bit>
#include <cstdint>
#include <limits>

#include "Algorithm.h"

using namespace std;

namespace satp::algorithms {
    class HyperLogLogPlusPlus final : public Algorithm {
    public:
        explicit HyperLogLogPlusPlus(uint32_t K);

        void process(uint32_t id) override;

        uint64_t count() override;

        void reset() override;

        string getName() override;

    private:
        uint32_t k;
        uint32_t numberOfBuckets; // nel paper coincide con m
        vector<uint8_t> bitmap; // bitmap
        double alphaM;

        static constexpr double ALPHA_16 = 0.673;
        static constexpr double ALPHA_32 = 0.697;
        static constexpr double ALPHA_64 = 0.709;
    };
} // namespace satp::algorithms
