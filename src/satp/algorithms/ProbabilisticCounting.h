#pragma once

#include <vector>
#include <bit>          // std::countr_zero (C++20)
#include <cstdint>
#include <limits>

#include "Algorithm.h"

namespace satp::algorithms {

    /**
     * Sketch probabilistico di Flajolet & Martin (1985) con un solo bitmap M.
     *
     *  - L  : numero di bit del bitmap (1≤L≤31 consigliato)
     *  - φ⁻¹: costante di correzione 1/φ ≈ 0.77351
     *
     * Complessità:
     *   process()  →  O(1)
     *   count()    →  O(L) (scansione del bitmap)
     *   memoria    →  O(L) bit
     */
    class ProbabilisticCounting final : public Algorithm {
    public:
        explicit ProbabilisticCounting(std::uint32_t L);

        void process(std::uint32_t id) override;
        [[nodiscard]]
        std::uint64_t count() const override;
        void reset() override;

    private:
        // ------------ hashing ---------------------------------------------------
        static std::uint32_t mix32(std::uint32_t x);   // MurmurHash3 finaliser
        [[nodiscard]]
        inline std::uint32_t uniformHash(std::uint32_t id) const;

        // ------------ bitmap helpers -------------------------------------------
        [[nodiscard]]
        std::uint32_t nextClearBit(std::uint32_t from) const;

        // ------------ members ---------------------------------------------------
        std::uint32_t          L_;          // lunghezza bitmap
        std::uint32_t          mask_;       // mantiene i L bit più bassi
        std::vector<bool>      bitmap_;     // M
        static constexpr double INV_PHI = 0.77351;
    };

} // namespace satp::algorithms
