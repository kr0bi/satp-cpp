#pragma once
#include <cstdint>
using namespace std;

namespace satp::algorithms {

    /**
     * @brief Interfaccia astratta per tutti gli algoritmi di stima della cardinalità.
     *
     * - process()  : inserisce un nuovo ID nel calcolo dello sketch;
     * - count()    : restituisce la stima corrente della cardinalità (o il conteggio esatto
     *                per gli algoritmi “naive”);
     * - reset()    : facoltativo, azzera lo stato interno (utile nei benchmark).
     *
     */
    class Algorithm {
    public:
        virtual ~Algorithm() = default;

        virtual void process(uint32_t id) = 0;

        virtual uint64_t count() = 0;

        virtual void reset() {}
    };

} // namespace satp::algorithms
