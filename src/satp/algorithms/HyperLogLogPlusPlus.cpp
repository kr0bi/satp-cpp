#include "HyperLogLogPlusPlus.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "hllpp_tables.h"
#include "satp/hashing.h"

using namespace std;

namespace satp::algorithms {
    HyperLogLogPlusPlus::HyperLogLogPlusPlus(uint32_t K)
        : p(K),
          m(0),
          mSparse(1u << SPARSE_P),
          format(Format::Sparse),
          registers(),
          alphaM(0.0),
          sumInversePowers(0.0),
          zeroRegisters(0),
          tmpSet(),
          sparseList(),
          sparseBits(0) {
        if (p < MIN_P || p > MAX_P) {
            throw invalid_argument("HLL++ requires p in [4, 18]");
        }

        m = 1u << p;
        switch (m) {
            case 16u: alphaM = ALPHA_16;
                break;
            case 32u: alphaM = ALPHA_32;
                break;
            case 64u: alphaM = ALPHA_64;
                break;
            default:
                alphaM = 0.7213 / (1.0 + 1.079 / static_cast<double>(m));
                break;
        }

        tmpSet.reserve(TMP_SET_FLUSH_SIZE * 2);
        reset();
    }

    void HyperLogLogPlusPlus::process(uint32_t id) {
        const uint64_t hash = util::hashing::splitmix64(id);
        if (format == Format::Normal) {
            addNormalHash(hash);
            return;
        }

        tmpSet.insert(encodeHash(hash));
        if (tmpSet.size() >= TMP_SET_FLUSH_SIZE) {
            flushTmpSetToSparseList();
            if (sparseBits > denseBits()) {
                convertSparseToNormal();
            }
        }
    }

    uint64_t HyperLogLogPlusPlus::count() {
        if (format == Format::Sparse) {
            flushTmpSetToSparseList();
            const double zeros = static_cast<double>(mSparse - sparseList.size());
            return static_cast<uint64_t>(linearCounting(static_cast<double>(mSparse), zeros));
        }

        const double raw = rawEstimateNormal();
        double corrected = raw;
        if (raw <= (5.0 * static_cast<double>(m))) {
            corrected = raw - estimateBias(raw);
            if (corrected < 0.0) {
                corrected = 0.0;
            }
        }

        double linear = corrected;
        if (zeroRegisters != 0u) {
            linear = linearCounting(static_cast<double>(m), static_cast<double>(zeroRegisters));
        }

        const double threshold = static_cast<double>(hllpp_tables::threshold_for_k(p));
        const double estimate = (linear <= threshold) ? linear : corrected;
        return static_cast<uint64_t>(estimate);
    }

    void HyperLogLogPlusPlus::reset() {
        format = Format::Sparse;
        registers.clear();
        sumInversePowers = 0.0;
        zeroRegisters = 0u;
        tmpSet.clear();
        sparseList.clear();
        sparseBits = 0u;
    }

    string HyperLogLogPlusPlus::getName() {
        return "HyperLogLog++";
    }

    uint32_t HyperLogLogPlusPlus::encodeHash(uint64_t hash) const {
        const uint32_t sparseIdx = static_cast<uint32_t>(hash >> (64u - SPARSE_P));
        const uint32_t idxTailBits = SPARSE_P - p;
        const uint32_t idxTailMask = (1u << idxTailBits) - 1u;

        if ((sparseIdx & idxTailMask) == 0u) {
            const uint64_t wPrime = hash & ((1ull << (64u - SPARSE_P)) - 1ull);
            const uint8_t rhoPrime = rho(wPrime, 64u - SPARSE_P);
            return (sparseIdx << 7u) | (static_cast<uint32_t>(rhoPrime) << 1u) | 1u;
        }
        return sparseIdx << 1u;
    }

    uint32_t HyperLogLogPlusPlus::sparseIndex(uint32_t encoded) const {
        return (encoded & 1u) ? (encoded >> 7u) : (encoded >> 1u);
    }

    uint8_t HyperLogLogPlusPlus::rhoFromEncoded(uint32_t encoded) const {
        const uint32_t idxTailBits = SPARSE_P - p;
        if (encoded & 1u) {
            const uint8_t rhoPrime = static_cast<uint8_t>((encoded >> 1u) & 0x3Fu);
            return static_cast<uint8_t>(rhoPrime + idxTailBits);
        }

        const uint32_t idxPrime = sparseIndex(encoded);
        const uint32_t prefix = idxPrime & ((1u << idxTailBits) - 1u);
        if (prefix == 0u) {
            return static_cast<uint8_t>(idxTailBits + 1u);
        }
        const uint32_t leading = static_cast<uint32_t>(countl_zero(prefix)) - (32u - idxTailBits);
        return static_cast<uint8_t>(leading + 1u);
    }

    pair<uint32_t, uint8_t> HyperLogLogPlusPlus::decodeHash(uint32_t encoded) const {
        const uint32_t idxPrime = sparseIndex(encoded);
        const uint32_t idx = idxPrime >> (SPARSE_P - p);
        return {idx, rhoFromEncoded(encoded)};
    }

    void HyperLogLogPlusPlus::flushTmpSetToSparseList() {
        if (tmpSet.empty()) {
            return;
        }

        vector<uint32_t> incoming;
        incoming.reserve(tmpSet.size());
        for (const uint32_t encoded: tmpSet) {
            incoming.push_back(encoded);
        }
        tmpSet.clear();

        auto cmpBySparseIndex = [this](uint32_t a, uint32_t b) {
            const uint32_t idxA = sparseIndex(a);
            const uint32_t idxB = sparseIndex(b);
            if (idxA != idxB) {
                return idxA < idxB;
            }
            const uint8_t rhoA = rhoFromEncoded(a);
            const uint8_t rhoB = rhoFromEncoded(b);
            if (rhoA != rhoB) {
                return rhoA > rhoB;
            }
            return a < b;
        };

        sort(incoming.begin(), incoming.end(), cmpBySparseIndex);

        vector<uint32_t> incomingUnique;
        incomingUnique.reserve(incoming.size());
        for (const uint32_t encoded: incoming) {
            if (incomingUnique.empty() || sparseIndex(incomingUnique.back()) != sparseIndex(encoded)) {
                incomingUnique.push_back(encoded);
            }
        }

        if (sparseList.empty()) {
            sparseList.swap(incomingUnique);
        } else {
            vector<uint32_t> merged;
            merged.reserve(sparseList.size() + incomingUnique.size());

            size_t i = 0;
            size_t j = 0;
            while (i < sparseList.size() && j < incomingUnique.size()) {
                const uint32_t idxA = sparseIndex(sparseList[i]);
                const uint32_t idxB = sparseIndex(incomingUnique[j]);
                if (idxA < idxB) {
                    merged.push_back(sparseList[i++]);
                } else if (idxB < idxA) {
                    merged.push_back(incomingUnique[j++]);
                } else {
                    const uint8_t rhoA = rhoFromEncoded(sparseList[i]);
                    const uint8_t rhoB = rhoFromEncoded(incomingUnique[j]);
                    merged.push_back((rhoB > rhoA) ? incomingUnique[j] : sparseList[i]);
                    ++i;
                    ++j;
                }
            }
            while (i < sparseList.size()) {
                merged.push_back(sparseList[i++]);
            }
            while (j < incomingUnique.size()) {
                merged.push_back(incomingUnique[j++]);
            }
            sparseList.swap(merged);
        }

        sparseBits = compressedSparseBits();
    }

    void HyperLogLogPlusPlus::convertSparseToNormal() {
        if (format == Format::Normal) {
            return;
        }

        flushTmpSetToSparseList();

        registers.assign(m, 0u);
        sumInversePowers = static_cast<double>(m);
        zeroRegisters = m;

        for (const uint32_t encoded: sparseList) {
            const auto [idx, r] = decodeHash(encoded);
            addNormalRegister(idx, r);
        }

        sparseList.clear();
        sparseBits = 0u;
        tmpSet.clear();
        format = Format::Normal;
    }

    void HyperLogLogPlusPlus::addNormalHash(uint64_t hash) {
        const uint32_t idx = static_cast<uint32_t>(hash >> (64u - p));
        const uint64_t w = hash << p;
        const uint8_t r = (w == 0u)
                              ? static_cast<uint8_t>((64u - p) + 1u)
                              : static_cast<uint8_t>(static_cast<uint32_t>(countl_zero(w)) + 1u);
        addNormalRegister(idx, r);
    }

    void HyperLogLogPlusPlus::addNormalRegister(uint32_t idx, uint8_t r) {
        const uint8_t old = registers[idx];
        if (r <= old) {
            return;
        }

        sumInversePowers += ldexp(1.0, -static_cast<int>(r)) - ldexp(1.0, -static_cast<int>(old));
        if (old == 0u) {
            --zeroRegisters;
        }
        registers[idx] = r;
    }

    size_t HyperLogLogPlusPlus::denseBits() const {
        return static_cast<size_t>(m) * 6u;
    }

    size_t HyperLogLogPlusPlus::compressedSparseBits() const {
        size_t bits = 0u;
        uint32_t previous = 0u;
        bool first = true;

        for (const uint32_t encoded: sparseList) {
            const uint32_t idx = sparseIndex(encoded);
            const uint32_t payload = (encoded & 1u) ? (encoded & 0x7Fu) : 0u;
            const uint32_t normalized = (idx << 7u) | payload;

            uint32_t delta = first ? normalized : (normalized - previous);
            previous = normalized;
            first = false;

            do {
                bits += 8u;
                delta >>= 7u;
            } while (delta != 0u);
        }
        return bits;
    }

    double HyperLogLogPlusPlus::rawEstimateNormal() const {
        if (sumInversePowers <= numeric_limits<double>::min()) {
            return 0.0;
        }
        return alphaM * static_cast<double>(m) * static_cast<double>(m) / sumInversePowers;
    }

    double HyperLogLogPlusPlus::estimateBias(double raw) const {
        const auto &table = hllpp_tables::table_for_k(p);
        if (table.empty()) {
            return 0.0;
        }

        const size_t neighbors = min(BIAS_K_NEIGHBORS, table.size());
        vector<pair<double, double> > nearest;
        nearest.reserve(neighbors);

        for (const auto &[rawPoint, biasPoint]: table) {
            const double dist = std::abs(rawPoint - raw);
            if (nearest.size() < neighbors) {
                nearest.emplace_back(dist, biasPoint);
                continue;
            }

            size_t worst = 0;
            for (size_t i = 1; i < nearest.size(); ++i) {
                if (nearest[i].first > nearest[worst].first) {
                    worst = i;
                }
            }
            if (dist < nearest[worst].first) {
                nearest[worst] = {dist, biasPoint};
            }
        }

        double sumBias = 0.0;
        for (const auto &[_, b]: nearest) {
            sumBias += b;
        }
        return sumBias / static_cast<double>(nearest.size());
    }

    double HyperLogLogPlusPlus::linearCounting(double buckets, double zeros) const {
        if (zeros <= 0.0) {
            return buckets;
        }
        return buckets * std::log(buckets / zeros);
    }

    uint8_t HyperLogLogPlusPlus::rho(uint64_t value, uint32_t width) {
        if (width == 0u || width > 64u) {
            throw invalid_argument("rho width out of range");
        }
        if (value == 0u) {
            return static_cast<uint8_t>(width + 1u);
        }
        if (width == 64u) {
            return static_cast<uint8_t>(static_cast<uint32_t>(countl_zero(value)) + 1u);
        }

        const uint64_t mask = (1ull << width) - 1ull;
        const uint64_t v = value & mask;
        if (v == 0u) {
            return static_cast<uint8_t>(width + 1u);
        }
        const uint32_t leading = static_cast<uint32_t>(countl_zero(v)) - (64u - width);
        return static_cast<uint8_t>(leading + 1u);
    }
} // namespace satp::algorithms
