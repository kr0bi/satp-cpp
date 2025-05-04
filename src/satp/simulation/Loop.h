#pragma once
#include <concepts>
#include <vector>
#include "satp/algorithms/Algorithm.h"

namespace satp::simulation {
    template<typename T>
    concept AlgorithmLike = std::derived_from<T, algorithms::Algorithm>;

    /**
     * Esegue uno stream di ID su un algoritmo di cardinalità.
     * @tparam A  una classe che implementa Algorithm (NaiveCounting, ProbabilisticCounting, …)
     */
    template<AlgorithmLike A>
    class Loop {
    public:

        Loop(A algorithm, std::vector<std::uint32_t> ids)
            : algorithm_(std::move(algorithm))
            , ids_(std::move(ids))
        {}

        std::uint64_t process()
        {
            for (std::uint32_t id : ids_) {
                algorithm_.process(id);
            }
            return algorithm_.count();
        }

        void reset()
        {
            algorithm_.reset();
            ids_.clear();
        }

    private:
        A algorithm_;
        std::vector<std::uint32_t> ids_;
    };

} // namespace satp::simulation
