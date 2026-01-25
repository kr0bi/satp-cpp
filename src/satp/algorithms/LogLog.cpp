#include "LogLog.h"
#include <cmath>
#include <stdexcept>

#include "satp/hashing.h"

using namespace std;

namespace satp::algorithms {
    LogLog::LogLog(uint32_t K, uint32_t L)
        : k(K),
          numberOfBuckets(1u << k),
          lengthOfBitMap(L),
          bitmap(1u << k, 0u) {
        if (k == 0) throw invalid_argument("k must be > 0");
        if (k >= 32) throw invalid_argument("k must be < 32");
        if (lengthOfBitMap <= k) throw invalid_argument("L must be > k");
        if (lengthOfBitMap > 32) throw invalid_argument("L must be <= 32");
    }

    void LogLog::process(uint32_t id) {
        const uint64_t hash64 = util::hashing::splitmix64(id);
        const uint32_t h32 = util::hashing::hash32_from_64(hash64);
        const uint32_t hash = (lengthOfBitMap >= 32)
                                  ? h32
                                  : (h32 & ((1u << lengthOfBitMap) - 1u));

        uint32_t firstKBits = hash >> (lengthOfBitMap - k); // primi k bit

        const uint32_t wbits = lengthOfBitMap - k;
        const uint32_t rem = hash & ((1u << wbits) - 1u); // restanti length - k bit
        const uint32_t b = (rem == 0)
                               ? (wbits + 1) // caso: tutti zeri =>  L+1 stando al paper
                               : (static_cast<uint32_t>(countl_zero(rem)) - (32u - wbits) + 1); // rho su wbits

        bitmap[firstKBits] = max(bitmap[firstKBits], b);
    }

    uint64_t LogLog::count() {
        double Z = 0.0;
        for (auto r: bitmap) Z += r;
        Z /= numberOfBuckets;
        return static_cast<uint64_t>(ALPHA_INF * numberOfBuckets * exp2(Z));
    }

    void LogLog::reset() {
        for (uint32_t i = 0; i < numberOfBuckets; ++i) {
            bitmap[i] = 0;
        }
    }

    string LogLog::getName() {
        return "LogLog";
    }
} // namespace satp::algorithms
