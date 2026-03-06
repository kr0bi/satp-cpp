#include "ProbabilisticCounting.h"
#include <cmath>
#include <stdexcept>

#include "satp/algorithms/AlgorithmCatalog.h"

using namespace std;

namespace satp::algorithms {
    ProbabilisticCounting::ProbabilisticCounting(
        uint32_t L,
        const hashing::HashFunction &hashFunction)
        : Algorithm(hashFunction),
          lengthBitMap(L),
          bitmap(0) {
        if (lengthBitMap == 0 || lengthBitMap > 31)
            throw invalid_argument("L must be in [1,31]");
    }

    void ProbabilisticCounting::process(uint32_t id) {
        const uint32_t hash = hashFunction().hash32(id) & ((1u << lengthBitMap) - 1u);
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
        return catalog::getNameBy("pc");
    }

    void ProbabilisticCounting::merge(const Algorithm &other) {
        const auto *typed = dynamic_cast<const ProbabilisticCounting *>(&other);
        if (typed == nullptr) {
            throw invalid_argument("ProbabilisticCounting merge requires ProbabilisticCounting");
        }
        merge(*typed);
    }

    void ProbabilisticCounting::merge(const ProbabilisticCounting &other) {
        if (lengthBitMap != other.lengthBitMap) {
            throw invalid_argument("ProbabilisticCounting merge requires same L");
        }
        bitmap |= other.bitmap;
    }
} // namespace satp::algorithms
