#pragma once
#include <cstdint>
#include <string>

#include "satp/hashing/HashFunction.h"

using namespace std;

namespace satp::algorithms {
    /**
     * @brief Interfaccia astratta per tutti gli algoritmi di stima della cardinalità.
     *
     * - process()  : inserisce un nuovo ID nel calcolo dello sketch;
     * - count()    : restituisce la stima corrente della cardinalità (o il conteggio esatto
     *                per gli algoritmi “naive”);
     * - reset()    : facoltativo, azzera lo stato interno (utile nei benchmark).
     * - merge()    : combina lo stato di un altro sketch compatibile.
     *
     */
    class Algorithm {
    public:
        explicit Algorithm(const hashing::HashFunction &hashFunction)
            : hashFunction_(&hashFunction) {
        }

        virtual ~Algorithm() = default;

        virtual void process(uint32_t id) = 0;

        virtual uint64_t count() = 0;

        virtual void merge(const Algorithm &other) = 0;

        virtual void reset() {
        }

        virtual string getName() = 0;

    protected:
        [[nodiscard]] const hashing::HashFunction &hashFunction() const {
            return *hashFunction_;
        }

    private:
        const hashing::HashFunction *hashFunction_ = nullptr;
    };
} // namespace satp::algorithms
