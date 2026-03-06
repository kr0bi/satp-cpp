#pragma once

#include <concepts>
#include <memory>
#include <vector>

#include "satp/ProgressBar.h"
#include "satp/algorithms/Algorithm.h"

using namespace std;

namespace satp::testsupport {
    template<typename T>
    concept AlgorithmLike = derived_from<T, algorithms::Algorithm>;

    template<AlgorithmLike A>
    class AlgorithmLoop {
    public:
        AlgorithmLoop(A algorithm, vector<uint32_t> ids, bool verbose = false)
            : algorithm_(std::move(algorithm))
              , ids_(std::move(ids))
              , verbose_(verbose) {
        }

        uint64_t process() {
            if (verbose_) {
                cerr << "\nAlgorithm: " << algorithm_.getName() << '\n';
            }

            unique_ptr<util::ProgressBar> bar;
            if (verbose_) {
                bar = make_unique<util::ProgressBar>(ids_.size());
            }
            for (uint32_t id : ids_) {
                algorithm_.process(id);
                if (bar) bar->tick();
            }

            if (bar) bar->finish();
            return algorithm_.count();
        }

    private:
        A algorithm_;
        vector<uint32_t> ids_;
        bool verbose_;
    };
} // namespace satp::testsupport
