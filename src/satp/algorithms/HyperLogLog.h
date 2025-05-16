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
        explicit HyperLogLog(uint32_t K, uint32_t L);

        void process(uint32_t id) override;

        uint64_t count() override;

        void reset() override;

        string getName() override;

    private:
        uint32_t k;
        uint32_t numberOfBuckets; // nel paper coincide con m
        uint32_t lengthOfBitMap; // lunghezza della bitmap, valore inutile, perché il paper assume che
        vector<uint32_t> bitmap; // bitmap
        double alphaM;

        static constexpr double ALPHA_16 = 0.673;
        static constexpr double ALPHA_32 = 0.697;
        static constexpr double ALPHA_64 = 0.709;
    };
} // namespace satp::algorithms
