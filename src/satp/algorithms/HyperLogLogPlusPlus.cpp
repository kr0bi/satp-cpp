#include "HyperLogLogPlusPlus.h"
#include <cmath>
#include <stdexcept>

#include "satp/hashing.h"

using namespace std;

namespace satp::algorithms {
    HyperLogLogPlusPlus::HyperLogLogPlusPlus(uint32_t K)
        : k(K),
          numberOfBuckets(1u << k),
          bitmap(numberOfBuckets, 0u),
          alphaM(0.7213 / (1 + 1.079 / static_cast<double>(numberOfBuckets))) {
    }

    void HyperLogLogPlusPlus::process(uint32_t id) {
        uint64_t hash = util::hashing::splitmix64(id);

        const uint32_t firstKBits = hash >> (64 - k); // primi k bit

        uint64_t rem = (hash << k); // restanti length - k bit
        uint8_t b = (rem == 0)
                        ? (64 - k) + 1 // caso: tutti zeri =>  L+1 stando al paper
                        : static_cast<std::uint8_t>(std::countl_zero(rem) + 1);; // zeri - k + 1

        bitmap[firstKBits] = max(bitmap[firstKBits], b);
    }

    uint64_t HyperLogLogPlusPlus::count() {
        double Z = 0.0;
        for (auto r: bitmap) Z += 1.0 / static_cast<double>(uint64_t{1} << r);
        Z /= numberOfBuckets;
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

        if (E <= ((5.0 / 2.0) * numberOfBuckets)) {
            int V = 0;
            for (auto r: bitmap) {
                if (r == 0) {
                    ++V;
                }
            }
            if (V != 0) {
                return static_cast<uint64_t>(numberOfBuckets * log2(static_cast<double>(numberOfBuckets) / V));
            }
        }
        return static_cast<uint64_t>(E);
    }

    void HyperLogLogPlusPlus::reset() {
        for (uint32_t i = 0; i < numberOfBuckets; ++i) {
            bitmap[i] = 0;
        }
    }

    string HyperLogLogPlusPlus::getName() {
        return "HyperLogLog";
    }
} // namespace satp::algorithms
