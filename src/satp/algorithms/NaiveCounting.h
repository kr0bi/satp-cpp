#pragma once
#include <vector>
#include <cstdint>
#include "Algorithm.h"

namespace satp::algorithms {

    /**
     * Implementazione “ingenua”:
     * mantiene un vettore di tutti gli ID distinti visti finora
     * e controlla linearmente se l'ID è già presente.
     *
     * Complessità:
     *   process()  ->  O(n)   (ricerca lineare)
     *   count()    ->  O(1)
     *   memoria    ->  O(n)   (n = distinti)
     */
    class NaiveCounting final : public Algorithm {
    public:
        void process(std::uint32_t id) override;
        [[nodiscard]]
        std::uint64_t  count() const override;
        void reset() override;

    private:
        std::vector<std::uint32_t> ids_;
    };

} // namespace satp::algorithms
