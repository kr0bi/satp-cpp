#pragma once

#include <bit>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

#include "Algorithm.h"

using namespace std;

namespace satp::algorithms {
    class HyperLogLogPlusPlus final : public Algorithm {
    public:
        // p = number of register index bits (m = 2^p registers).
        // Follows HyperLogLog++ as described by Heule et al. for p in [4, 18].
        explicit HyperLogLogPlusPlus(
            uint32_t K,
            const hashing::HashFunction &hashFunction);

        void process(uint32_t id) override;

        uint64_t count() override;

        void reset() override;

        string getName() override;

        void merge(const Algorithm &other) override;

        void merge(const HyperLogLogPlusPlus &other);

        [[nodiscard]] HyperLogLogPlusPlus reducedTo(uint32_t targetP) const;

        [[nodiscard]] HyperLogLogPlusPlus reducedToNaive(uint32_t targetP) const;

    private:
        enum class Format {
            Sparse,
            Normal
        };

        static constexpr uint32_t MIN_P = 4;
        static constexpr uint32_t MAX_P = 18;
        static constexpr uint32_t SPARSE_P = 25;
        static constexpr size_t BIAS_K_NEIGHBORS = 6;
        static constexpr size_t TMP_SET_FLUSH_SIZE = 1u << 12;

        uint32_t p;
        uint32_t m;
        uint32_t mSparse;
        Format format;

        vector<uint8_t> registers;
        double alphaM;
        double sumInversePowers;
        uint32_t zeroRegisters;

        unordered_set<uint32_t> tmpSet;
        vector<uint32_t> sparseList;
        size_t sparseBits;

        static constexpr double ALPHA_16 = 0.673;
        static constexpr double ALPHA_32 = 0.697;
        static constexpr double ALPHA_64 = 0.709;

        [[nodiscard]] uint32_t encodeHash(uint64_t hash) const;
        [[nodiscard]] uint32_t sparseIndex(uint32_t encoded) const;
        [[nodiscard]] uint8_t rhoFromEncoded(uint32_t encoded) const;
        [[nodiscard]] pair<uint32_t, uint8_t> decodeHash(uint32_t encoded) const;
        [[nodiscard]] HyperLogLogPlusPlus reducePrecision(uint32_t targetP,
                                                          bool correctDroppedBits) const;

        void flushTmpSetToSparseList();
        void convertSparseToNormal();
        void addNormalHash(uint64_t hash);
        void addNormalRegister(uint32_t idx, uint8_t rho);

        [[nodiscard]] size_t denseBits() const;
        [[nodiscard]] size_t compressedSparseBits() const;
        [[nodiscard]] double rawEstimateNormal() const;
        [[nodiscard]] double estimateBias(double raw) const;
        [[nodiscard]] double linearCounting(double buckets, double zeros) const;

        [[nodiscard]] static uint8_t rho(uint64_t value, uint32_t width);
    };
} // namespace satp::algorithms
