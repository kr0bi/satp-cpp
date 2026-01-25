#include "ProbabilisticCounting.h"
#include <cmath>
#include <stdexcept>

#include "satp/hashing.h"

using namespace std;

namespace satp::algorithms {
    ProbabilisticCounting::ProbabilisticCounting(uint32_t L)
        : lengthBitMap(L),
          bitmap(0) {
        if (lengthBitMap == 0 || lengthBitMap > 31)
            throw invalid_argument("L must be in [1,31]");
    }

    void ProbabilisticCounting::process(uint32_t id) {
        const uint64_t hash64 = util::hashing::splitmix64(id);
        const uint32_t h32 = util::hashing::hash32_from_64(hash64);
        const uint32_t hash = h32 & ((1u << lengthBitMap) - 1u);
        if (hash == 0) return;

        uint32_t rightMostOneBit = countr_zero(hash);
        if (rightMostOneBit < lengthBitMap)
            bitmap |= 1u << rightMostOneBit;
    }

    uint64_t ProbabilisticCounting::count() {
        uint32_t idxRightmostZero = countr_one(bitmap);
        if (idxRightmostZero > lengthBitMap) idxRightmostZero = lengthBitMap;
        constexpr double PHI_INV = 1.0 / INV_PHI;
        double q = ldexp(1.0, idxRightmostZero) * PHI_INV;
        return static_cast<uint64_t>(q);
    }

    void ProbabilisticCounting::reset() {
        bitmap = 0;
    }

    string ProbabilisticCounting::getName() {
        return "Probabilistic Counting";
    }
} // namespace satp::algorithms
