#pragma once
#include <concepts>
#include <memory>
#include <vector>

#include "satp/ProgressBar.h"
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
        Loop(A algorithm, std::vector<std::uint32_t> ids, bool verbose = false)
            : algorithm_(std::move(algorithm))
              , ids_(std::move(ids))
              , verbose_(verbose) {
        }

        std::uint64_t process() {
            if (verbose_) {
                std::cerr << "\nAlgorithm: " << algorithm_.getName() << '\n';
            }

            std::unique_ptr<util::ProgressBar> bar;
            if (verbose_) {
                bar = std::make_unique<util::ProgressBar>(ids_.size());
            }
            for (std::uint32_t id: ids_) {
                algorithm_.process(id);
                if (bar) bar->tick();
            }

            if (bar) bar->finish();
            return algorithm_.count();
        }

    private:
        A algorithm_;
        std::vector<std::uint32_t> ids_;
        bool verbose_;
    };
} // namespace satp::simulation
