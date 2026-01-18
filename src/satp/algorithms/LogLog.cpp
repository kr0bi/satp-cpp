#include "LogLog.h"
#include <cmath>
#include <stdexcept>

#include "satp/hashing.h"

using namespace std;

namespace satp::algorithms {
    namespace {
        constexpr double ALPHA_INF = 0.39701;

        double alpha_for(uint32_t /*m*/) {
            return ALPHA_INF;
        }
    } // namespace

    LogLog::LogLog(uint32_t K, uint32_t L)
        : k(K),
          numberOfBuckets(1u << k),
          lengthOfBitMap(L),
          bitmap(1u << k, 0u) {
    }

    void LogLog::process(uint32_t id) {
        uint32_t hash = util::hashing::uniform_hash(id, lengthOfBitMap);

        uint32_t firstKBits = hash >> (lengthOfBitMap - k); // primi k bit

        uint32_t rem = hash & ((1u << (lengthOfBitMap - k)) - 1); // restanti length - k bit
        uint32_t b = (rem == 0)
                         ? (lengthOfBitMap - k) + 1 // caso: tutti zeri =>  L+1 stando al paper
                         : countl_zero(rem) - k + 1; // zeri - k + 1

        bitmap[firstKBits] = max(bitmap[firstKBits], b);
    }

    uint64_t LogLog::count() {
        double Z = 0.0;
        for (auto r: bitmap) Z += r;
        Z /= numberOfBuckets;
        const double alpha = alpha_for(numberOfBuckets);
        return static_cast<uint64_t>(alpha * numberOfBuckets * exp2(Z));
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
