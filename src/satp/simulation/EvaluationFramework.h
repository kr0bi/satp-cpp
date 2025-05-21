#pragma once

#include <vector>
#include <cstdint>
#include <filesystem>
#include <numeric>      // accumulate
#include <utility>      // forward

#include "../Utils.h"
#include "../algorithms/Algorithm.h"
#include "satp/ProgressBar.h"
#include "satp/io/BinaryIO.h"

using namespace std;

namespace satp::evaluation {
    struct Stats {
        double difference = 0.0;
        double mean = 0.0;
        double variance = 0.0;
        double bias = 0.0;
    };

    class EvaluationFramework {
    public:
        explicit EvaluationFramework(vector<uint32_t> data)
            : valori(move(data)),
              numElementiDistintiEffettivi(satp::utils::count_distinct(valori)) {
        }

        EvaluationFramework(const std::filesystem::path &filePath,
                            std::size_t runs,
                            std::size_t sampleSize,
                            std::size_t numberOfElements,
                            std::size_t highestNumber) {
            try {
                satp::io::loadDataset(filePath, valori, sottoInsiemi);
                if (sottoInsiemi.size() != runs || (!sottoInsiemi.empty() && sottoInsiemi[0].size() != sampleSize))
                    throw std::runtime_error("Cached file has wrong shape");
                std::cout << "[cache] dataset e subset caricati da: " << filePath << '\n';
            } catch (...) {
                std::cout << "[cache] assente o incompatibile, genero ex-novo\n";
                valori = satp::utils::getRandomNumbers(numberOfElements, highestNumber);
                numElementiDistintiEffettivi = satp::utils::count_distinct(valori);
                rng.seed(std::random_device{}());
                ensureSubsets(runs, sampleSize); // genera subset
                satp::io::saveDataset(filePath, valori, sottoInsiemi);
                std::cout << "[cache] salvato in: " << filePath << '\n';
            }
        }

        /**
         * Valuta un algoritmo su "runs" passate.
         * Args... sono inoltrati al costruttore dell'algoritmo.
         */
        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluate(size_t runs,
                                     size_t sampleSize,
                                     Args &&... ctorArgs) const {
            if (runs == 0 || sampleSize == 0) return {};

            cout << "Creazione dei sottoinsiemi\n";
            ensureSubsets(runs, sampleSize);

            using satp::util::ProgressBar;
            ProgressBar bar{runs * sampleSize, cout, 50, 10'000};

            vector<double> estimates;
            estimates.reserve(runs);

            vector<double> truths;
            truths.reserve(runs);

            for (size_t r = 0; r < runs; ++r) {
                const auto &sottoInsieme = sottoInsiemi[r];
                Algo algo(forward<Args>(ctorArgs)...);

                for (auto v: sottoInsieme) {
                    algo.process(v);
                    bar.tick();
                }
                estimates.push_back(static_cast<double>(algo.count()));
                truths.push_back(static_cast<double>(utils::count_distinct(sottoInsieme)));
            }

            bar.finish();
            cout.flush();

            // ---- statistiche ---------------------------------------------
            const auto mean = reduce(estimates.begin(), estimates.end()) / runs;
            const auto gtMean = reduce(truths.begin(), truths.end()) / runs;

            double varAcc = 0.0;
            for (double e: estimates) varAcc += (e - mean) * (e - mean);
            const double variance = varAcc / (runs - 1);

            const double bias = mean - gtMean;
            const double difference = abs(bias);

            return {difference, mean, variance, bias};
        }

        [[nodiscard]] size_t getNumElementiDistintiEffettivi() const noexcept { return numElementiDistintiEffettivi; }
        [[nodiscard]] const vector<uint32_t> &data() const noexcept { return valori; }

        /**
         * Viene chiamato al primo evaluate() e riutilizza i sotto-insiemi per le chiamate successive dello stesso EvaluationFramework
         * @param runs
         * @param sampleSize
         */
        void ensureSubsets(size_t runs, size_t sampleSize) const {
            if (!sottoInsiemi.empty()) return;

            sottoInsiemi.reserve(runs);
            for (std::size_t r = 0; r < runs; ++r) {
                std::vector<std::uint32_t> subset;
                subset.reserve(sampleSize);
                std::sample(valori.begin(), valori.end(),
                            std::back_inserter(subset),
                            sampleSize, rng);
                sottoInsiemi.push_back(std::move(subset));
            }
        }

    private:
        vector<uint32_t> valori;
        size_t numElementiDistintiEffettivi;

        mutable vector<vector<uint32_t> > sottoInsiemi;
        mutable size_t cachedRuns = 0;
        mutable size_t cachedSampleSize = 0;
        mutable mt19937 rng{random_device{}()};
    };
} // namespace satp::evaluation
