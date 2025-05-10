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
    }

    void LogLog::process(uint32_t id) {
        uint32_t hash = util::hashing::uniform_hash(id, lengthOfBitMap);
        uint32_t firstKBit = hash >> (32 - k);
        uint32_t leftMostOneBit = 31 - countl_zero(firstKBit);
        bitmap[firstKBit] = max(bitmap[firstKBit], leftMostOneBit);
    }

    uint64_t LogLog::count() {
        uint32_t Z = 0;
        for (uint32_t i = 0; i < numberOfBuckets; ++i) {
            Z += bitmap[i];
        }
        Z = Z / numberOfBuckets;
        return static_cast<uint64_t>((FIRST_PHI * numberOfBuckets - SECOND_PHI) * (1u << Z));
    }

    void LogLog::reset() {
        for (uint32_t i = 0; i < numberOfBuckets; ++i) {
            bitmap[i] = 0;
        }
    }
} // namespace satp::algorithms
