#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "Algorithm.h"

using namespace std;

namespace satp::algorithms {
    class HyperLogLogPlusPlus final : public Algorithm {
    public:
        // k = number of register index bits (m = 2^k registers)
        explicit HyperLogLogPlusPlus(uint32_t K);

        void process(uint32_t id) override;

        uint64_t count() override;

        void reset() override;

        string getName() override;

    private:
        uint32_t k;
        size_t numberOfBuckets; // nel paper coincide con m
        vector<uint8_t> bitmap; // bitmap
        double alphaM;
        double sumInversePowers; // \sum_j 2^{-M[j]}
        size_t zeroRegisters;

        static constexpr double ALPHA_16 = 0.673;
        static constexpr double ALPHA_32 = 0.697;
        static constexpr double ALPHA_64 = 0.709;

        static double interpolateBias(double raw, std::uint32_t k);
    };
} // namespace satp::algorithms
