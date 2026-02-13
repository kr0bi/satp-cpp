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
        explicit HyperLogLogPlusPlus(uint32_t K);

        void process(uint32_t id) override;

        uint64_t count() override;

        void reset() override;

        string getName() override;

    private:
        enum class Format {
            Sparse,
            Normal
        };

        static constexpr std::uint32_t MIN_P = 4;
        static constexpr std::uint32_t MAX_P = 18;
        static constexpr std::uint32_t SPARSE_P = 25;
        static constexpr std::size_t BIAS_K_NEIGHBORS = 6;
        static constexpr std::size_t TMP_SET_FLUSH_SIZE = 1u << 12;

        std::uint32_t p;
        std::uint32_t m;
        std::uint32_t mSparse;
        Format format;

        std::vector<std::uint8_t> registers;
        double alphaM;
        double sumInversePowers;
        std::uint32_t zeroRegisters;

        std::unordered_set<std::uint32_t> tmpSet;
        std::vector<std::uint32_t> sparseList;
        std::size_t sparseBits;

        static constexpr double ALPHA_16 = 0.673;
        static constexpr double ALPHA_32 = 0.697;
        static constexpr double ALPHA_64 = 0.709;

        [[nodiscard]] std::uint32_t encodeHash(std::uint64_t hash) const;
        [[nodiscard]] std::uint32_t sparseIndex(std::uint32_t encoded) const;
        [[nodiscard]] std::uint8_t rhoFromEncoded(std::uint32_t encoded) const;
        [[nodiscard]] std::pair<std::uint32_t, std::uint8_t> decodeHash(std::uint32_t encoded) const;

        void flushTmpSetToSparseList();
        void convertSparseToNormal();
        void addNormalHash(std::uint64_t hash);
        void addNormalRegister(std::uint32_t idx, std::uint8_t rho);

        [[nodiscard]] std::size_t denseBits() const;
        [[nodiscard]] std::size_t compressedSparseBits() const;
        [[nodiscard]] double rawEstimateNormal() const;
        [[nodiscard]] double estimateBias(double raw) const;
        [[nodiscard]] double linearCounting(double buckets, double zeros) const;

        [[nodiscard]] static std::uint8_t rho(std::uint64_t value, std::uint32_t width);
    };
} // namespace satp::algorithms
