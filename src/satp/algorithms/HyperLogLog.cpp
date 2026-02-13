#include "HyperLogLog.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "satp/hashing.h"

using namespace std;

namespace satp::algorithms {
    HyperLogLog::HyperLogLog(uint32_t K, uint32_t L)
        : k(K),
          numberOfBuckets(1u << k),
          lengthOfBitMap(L),
          bitmap(1u << k, 0u),
          alphaM(0.7213 / (1 + 1.079 / pow(2.0, (double) k))),
          sumInversePowers(static_cast<double>(1u << k)),
          zeroRegisters(1u << k) {
        if (k == 0) throw invalid_argument("k must be > 0");
        if (k >= 32) throw invalid_argument("k must be < 32");
        if (lengthOfBitMap <= k) throw invalid_argument("L must be > k");
        if (lengthOfBitMap > 32) throw invalid_argument("L must be <= 32");
    }

    void HyperLogLog::process(uint32_t id) {
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

        const uint32_t old = bitmap[firstKBits];
        if (b > old) {
            sumInversePowers += ldexp(1.0, -static_cast<int>(b)) - ldexp(1.0, -static_cast<int>(old));
            if (old == 0u) {
                --zeroRegisters;
            }
            bitmap[firstKBits] = b;
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
} // namespace satp::algorithms
