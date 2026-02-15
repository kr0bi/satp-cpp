#include "LogLog.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "satp/hashing.h"

using namespace std;

namespace satp::algorithms {
    namespace {
        constexpr uint32_t LOGLOG_MIN_K = 4;
        constexpr uint32_t LOGLOG_MAX_K = 16;
        constexpr uint32_t LOGLOG_PAPER_L = 32;

        [[nodiscard]] uint32_t validateAndBucketCount(uint32_t K, uint32_t L) {
            if (L != LOGLOG_PAPER_L) {
                throw invalid_argument("LogLog paper-strict requires L = 32");
            }
            if (K < LOGLOG_MIN_K || K > LOGLOG_MAX_K) {
                throw invalid_argument("LogLog paper-strict requires k in [4,16]");
            }
            return (1u << K);
        }
    } // namespace

    LogLog::LogLog(uint32_t K, uint32_t L)
        : k(K),
          numberOfBuckets(validateAndBucketCount(K, L)),
          lengthOfBitMap(L),
          bitmap(numberOfBuckets, 0u),
          sumRegisters(0.0) {}

    void LogLog::process(uint32_t id) {
        const uint64_t hash64 = util::hashing::splitmix64(id);
        const uint32_t h32 = util::hashing::hash32_from_64(hash64);
        const uint32_t hash = h32;

        const uint32_t firstKBits = hash >> (lengthOfBitMap - k);
        // remaining (32-k) bits shifted to MSB side; rho = leading zeros + 1.
        const uint32_t rem = hash << k;
        const uint32_t wbits = lengthOfBitMap - k;
        const uint32_t b = (rem == 0u) ? (wbits + 1u) : (static_cast<uint32_t>(countl_zero(rem)) + 1u);

        const uint32_t old = bitmap[firstKBits];
        if (b > old) {
            bitmap[firstKBits] = static_cast<uint8_t>(b);
            sumRegisters += static_cast<double>(b - old);
        }
    }

    uint64_t LogLog::count() {
        const double Z = sumRegisters / static_cast<double>(numberOfBuckets);
        return static_cast<uint64_t>(ALPHA_INF * numberOfBuckets * exp2(Z));
    }

    void LogLog::reset() {
        ranges::fill(bitmap, 0u);
        sumRegisters = 0.0;
    }

    string LogLog::getName() {
        return "LogLog";
    }
} // namespace satp::algorithms
