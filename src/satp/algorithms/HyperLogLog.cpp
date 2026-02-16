#include "HyperLogLog.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "satp/hashing.h"

using namespace std;

namespace satp::algorithms {
    namespace {
        constexpr uint32_t HLL_MIN_K = 4;
        constexpr uint32_t HLL_MAX_K = 16;
        constexpr uint32_t HLL_PAPER_L = 32;

        [[nodiscard]] uint32_t validateAndBucketCount(uint32_t K, uint32_t L) {
            if (L != HLL_PAPER_L) {
                throw invalid_argument("HyperLogLog paper-strict requires L = 32");
            }
            if (K < HLL_MIN_K || K > HLL_MAX_K) {
                throw invalid_argument("HyperLogLog paper-strict requires k in [4,16]");
            }
            return (1u << K);
        }
    } // namespace

    HyperLogLog::HyperLogLog(uint32_t K, uint32_t L)
        : k(K),
          numberOfBuckets(validateAndBucketCount(K, L)),
          lengthOfBitMap(L),
          bitmap(numberOfBuckets, 0u),
          alphaM(0.7213 / (1 + 1.079 / pow(2.0, (double) k))),
          sumInversePowers(static_cast<double>(numberOfBuckets)),
          zeroRegisters(numberOfBuckets) {}

    void HyperLogLog::process(uint32_t id) {
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
            sumInversePowers += ldexp(1.0, -static_cast<int>(b)) - ldexp(1.0, -static_cast<int>(old));
            if (old == 0u) {
                --zeroRegisters;
            }
            bitmap[firstKBits] = static_cast<uint8_t>(b);
        }
    }

    uint64_t HyperLogLog::count() {
        double Z = sumInversePowers / static_cast<double>(numberOfBuckets);
        Z = 1.0 / Z;
        double E = 0.0;
        switch (k) {
            case 4: E = ALPHA_16 * numberOfBuckets * Z;
                break;
            case 5: E = ALPHA_32 * numberOfBuckets * Z;
                break;
            case 6:
                E = ALPHA_64 * numberOfBuckets * Z;
                break;
            default:
                E = alphaM * numberOfBuckets * Z;
                break;
        }

        if (E <= (2.5 * numberOfBuckets)) { // 5.0/2.0
            const int V = static_cast<int>(zeroRegisters);
            if (V != 0) {
                return static_cast<uint64_t>(numberOfBuckets * log(static_cast<double>(numberOfBuckets) / V));
            }
            return static_cast<uint64_t>(E);
        } else if (E <= ((1.0 / 30.0) * ldexp(1.0, 32))) { // 2^32
            return static_cast<uint64_t>(E);
        } else {
            const double two_pow_32 = ldexp(1.0, 32); // 2^32
            return static_cast<uint64_t>(-two_pow_32 * log(1 - (E / two_pow_32)));
        }
    }

    void HyperLogLog::reset() {
        std::fill(bitmap.begin(), bitmap.end(), 0u);
        sumInversePowers = static_cast<double>(numberOfBuckets);
        zeroRegisters = numberOfBuckets;
    }

    string HyperLogLog::getName() {
        return "HyperLogLog";
    }

    void HyperLogLog::merge(const Algorithm &other) {
        const auto *typed = dynamic_cast<const HyperLogLog *>(&other);
        if (typed == nullptr) {
            throw invalid_argument("HyperLogLog merge requires HyperLogLog");
        }
        merge(*typed);
    }

    void HyperLogLog::merge(const HyperLogLog &other) {
        if (k != other.k || lengthOfBitMap != other.lengthOfBitMap || numberOfBuckets != other.numberOfBuckets) {
            throw invalid_argument("HyperLogLog merge requires same k and L");
        }

        for (uint32_t i = 0; i < numberOfBuckets; ++i) {
            bitmap[i] = std::max(bitmap[i], other.bitmap[i]);
        }

        sumInversePowers = 0.0;
        zeroRegisters = 0u;
        for (const auto reg : bitmap) {
            sumInversePowers += ldexp(1.0, -static_cast<int>(reg));
            if (reg == 0u) {
                ++zeroRegisters;
            }
        }
    }
} // namespace satp::algorithms
