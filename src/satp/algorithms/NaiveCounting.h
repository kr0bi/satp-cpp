#pragma once
#include <vector>
#include <cstdint>
#include <set>

#include "Algorithm.h"

using namespace std;

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
        void process(uint32_t id) override;

        uint64_t count() override;

        void reset() override;

        string getName() override;

    private:
        set<uint32_t> ids;
    };
} // namespace satp::algorithms
