#pragma once

#include <vector>
#include <cstdint>
#include <numeric>      // accumulate
#include <utility>      // forward

#include "../Utils.h"
#include "../algorithms/Algorithm.h"
#include "satp/ProgressBar.h"

namespace satp::evaluation {
    struct Stats {
        double difference = 0.0;
        double mean = 0.0; // media delle stime
        double variance = 0.0; // varianza
        double bias = 0.0; // mean  − ground‑truth
    };

    // -----------------------------------------------------------------------
    // EvaluationFramework: genera una sola volta il dataset randomico e il
    // conteggio esatto (groundTruth), poi esegue ripetutamente l'algoritmo per
    // stimare media, varianza e bias.
    // -----------------------------------------------------------------------
    class EvaluationFramework {
    public:
        explicit EvaluationFramework(std::vector<std::uint32_t> data)
            : data_(std::move(data)),
              groundTruth_(satp::utils::count_distinct(data_)) {
        }

        /**
         * Valuta un algoritmo su "runs" passate.
         * Args... sono inoltrati al costruttore dell'algoritmo.
         */
        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluate(std::size_t runs, Args &&... ctorArgs) const {
            if (runs == 0) return {};

            using satp::util::ProgressBar;
            const std::size_t totalTicks = runs * data_.size();
            ProgressBar bar{totalTicks, std::cout, 50, 10'000}; // updateEvery=10k per non saturare la TTY

            std::vector<double> estimates;
            estimates.reserve(runs);

            for (std::size_t r = 0; r < runs; ++r) {
                Algo algo(std::forward<Args>(ctorArgs)...);
                for (auto v: data_) {
                    algo.process(v);
                    bar.tick(); // 1 tick per elemento processato
                }
                estimates.push_back(static_cast<double>(algo.count()));
            }

            bar.finish(); // riga finale “100 % | elapsed …”
            std::cout.flush(); // assicura l’ordine con le stampe successive

            // ---- statistiche ---------------------------------------------
            const double mean = std::accumulate(estimates.begin(), estimates.end(), 0.0) / runs;
            const bool differencePositive = mean > groundTruth_;
            const double difference = differencePositive
                                          ? mean - static_cast<double>(groundTruth_)
                                          : static_cast<double>(groundTruth_) - mean;

            double varAcc = 0.0;
            for (double e: estimates) varAcc += (e - mean) * (e - mean);
            const double variance = varAcc / runs; // popolazione

            const double bias = mean - static_cast<double>(groundTruth_);

            return {difference, mean, variance, bias};
        }

        [[nodiscard]] std::size_t ground_truth() const noexcept { return groundTruth_; }
        [[nodiscard]] const std::vector<std::uint32_t> &data() const noexcept { return data_; }

    private:
        std::vector<std::uint32_t> data_;
        std::size_t groundTruth_;
    };
} // namespace satp::evaluation
