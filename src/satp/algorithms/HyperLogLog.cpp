#include "HyperLogLog.h"
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
          alphaM(0.7213 / (1 + 1.079 / pow(2.0, (double) k))) {
    }

    void HyperLogLog::process(uint32_t id) {
        uint32_t hash = util::hashing::uniform_hash(id, lengthOfBitMap);

        uint32_t firstKBits = hash >> (lengthOfBitMap - k); // primi k bit

        uint32_t rem = hash & ((1u << (lengthOfBitMap - k)) - 1); // restanti length - k bit
        uint32_t b = (rem == 0)
                         ? (lengthOfBitMap - k) + 1 // caso: tutti zeri =>  L+1 stando al paper
                         : countl_zero(rem) - k + 1; // zeri - k + 1

        bitmap[firstKBits] = max(bitmap[firstKBits], b);
    }

    uint64_t HyperLogLog::count() {
        double Z = 0.0;
        for (auto r: bitmap) Z += ldexp(1.0, -static_cast<int>(r)); // 1u << r
        Z /= numberOfBuckets;
        Z = pow(Z, -1.0);
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
            int V = 0;
            for (auto r: bitmap) {
                if (r == 0) {
                    V++;
                }
            }
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
        for (uint32_t i = 0; i < numberOfBuckets; ++i) {
            bitmap[i] = 0;
        }
    }

    string HyperLogLog::getName() {
        return "HyperLogLog";
    }
} // namespace satp::algorithms
