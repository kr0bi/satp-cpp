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

        /* ---- bias table K = 16 (demo) ---- */
        static constexpr std::array<std::pair<double, double>, 11> bias16_ = {
            {
                {46'000, 1100}, {60'000, 870}, {90'000, 540}, {120'000, 390},
                {150'000, 300}, {180'000, 240}, {200'000, 200}, {220'000, 170},
                {240'000, 150}, {260'000, 135}, {290'000, 120}
            }
        };

        static double interpolateBias(double raw, std::uint32_t k);
    };
} // namespace satp::algorithms
