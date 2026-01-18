#include "HyperLogLogPlusPlus.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "hllpp_tables.h"
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
                        : static_cast<uint8_t>(countl_zero(rem) + 1);; // zeri - k + 1

        bitmap[firstKBits] = max(bitmap[firstKBits], b);
    }

    uint64_t HyperLogLogPlusPlus::count() {
        double Z = 0.0;
        for (auto r: bitmap) Z += ldexp(1.0, -static_cast<int>(r));
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

        if (k == 16) {
            const double m = numberOfBuckets;
            const double lcThreshold = 0.7029 * m; //  ≈ 11 500 / 16 384
            const double biasThreshold = 3.723 * m; //  ≈ 61 000 / 16 384
            /* ---- 1)  Linear-Counting range ---------------------------- */
            if (E < lcThreshold) {
                uint32_t V = std::count(bitmap.begin(), bitmap.end(), 0);
                return static_cast<uint64_t>(m * log(m / static_cast<double>(V)));
            }
            /* ---- 2)  Bias-corrected range ----------------------------- */
            if (E < biasThreshold) {
                const double bias = interpolateBias(E, k);
                return static_cast<uint64_t>(E - bias);
            }

            /* ---- 3) High range → use raw E --------------------------- */
            return static_cast<uint64_t>(E);
        }

        if (E <= ((5.0 / 2.0) * numberOfBuckets)) {
            int V = 0;
            for (auto r: bitmap) {
                if (r == 0) {
                    ++V;
                }
            }
            if (V != 0) {
                return static_cast<uint64_t>(numberOfBuckets * log(static_cast<double>(numberOfBuckets) / V));
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
        return "HyperLogLog++";
    }

    double HyperLogLogPlusPlus::interpolateBias(double raw, uint32_t k) {
        const auto &arr = hllpp_tables::table_for_k(k);
        if (raw <= arr.front().first) return arr.front().second;
        if (raw >= arr.back().first) return arr.back().second;

        auto hi = lower_bound(arr.begin(), arr.end(), raw,
                              [](auto p, double v) { return p.first < v; });
        auto lo = prev(hi);

        const double x1 = lo->first, y1 = lo->second;
        const double x2 = hi->first, y2 = hi->second;
        const double t = (raw - x1) / (x2 - x1);
        return y1 + t * (y2 - y1); // linear interpolation
    }
} // namespace satp::algorithms
