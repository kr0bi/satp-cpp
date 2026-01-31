#pragma once

#include <algorithm>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>      // accumulate
#include <random>
#include <string>
#include <unordered_set>
#include <utility>      // forward
#include <cmath>

#include "../algorithms/Algorithm.h"
#include "satp/ProgressBar.h"
#include "satp/io/TextDatasetIO.h"

using namespace std;

namespace satp::evaluation {
    struct Dataset {
        vector<uint32_t> values;
        size_t distinct_count = 0;
    };

    struct Stats {
        double difference = 0.0;
        double mean = 0.0;
        double variance = 0.0;
        double bias = 0.0;
        double mean_relative_error = 0.0;
        double bias_relative = 0.0;
        double rmse = 0.0;
        double mae = 0.0;
        double stddev = 0.0;
        double rse_observed = 0.0;
    };

    class EvaluationFramework {
    public:
        static constexpr uint32_t DEFAULT_SEED = 5489u;

        explicit EvaluationFramework(Dataset dataset,
                                     uint32_t seed = DEFAULT_SEED)
            : valori(std::move(dataset.values)),
              numElementiDistintiEffettivi(dataset.distinct_count),
              seed(seed),
              rng(seed) {
        }

        explicit EvaluationFramework(vector<uint32_t> data,
                                     size_t distinctCount,
                                     uint32_t seed = DEFAULT_SEED)
            : valori(std::move(data)),
              numElementiDistintiEffettivi(distinctCount),
              seed(seed),
              rng(seed) {
        }

        explicit EvaluationFramework(const filesystem::path &filePath,
                                     uint32_t seed = DEFAULT_SEED)
            : seed(seed),
              rng(seed) {
            const auto info = satp::io::loadTextDataset(filePath, valori);
            numElementiDistintiEffettivi = info.distinct_elements;
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

            double absErrSum = 0.0;
            double sqErrSum = 0.0;
            double absRelErrSum = 0.0;

            for (size_t r = 0; r < runs; ++r) {
                const auto &sottoInsieme = sottoInsiemi[r];
                Algo algo(std::forward<Args>(ctorArgs)...);

                for (auto v: sottoInsieme) {
                    algo.process(v);
                    bar.tick();
                }
                const double estimate = static_cast<double>(algo.count());
                const double truth = static_cast<double>(exactDistinctCount(sottoInsieme));
                estimates.push_back(estimate);
                truths.push_back(truth);

                const double err = estimate - truth;
                absErrSum += std::abs(err);
                sqErrSum += err * err;
                if (truth > 0.0) {
                    absRelErrSum += std::abs(err) / truth;
                }
            }

            bar.finish();
            cout.flush();

            // ---- statistiche ---------------------------------------------
            const auto mean = reduce(estimates.begin(), estimates.end()) / runs;
            const auto gtMean = reduce(truths.begin(), truths.end()) / runs;

            double varAcc = 0.0;
            for (double e: estimates) varAcc += (e - mean) * (e - mean);
            const double variance = (runs > 1) ? (varAcc / (runs - 1)) : 0.0;
            const double stddev = std::sqrt(variance);

            const double bias = mean - gtMean;
            const double difference = abs(bias);
            const double biasRelative = (gtMean != 0.0) ? (bias / gtMean) : 0.0;

            const double meanRelativeError = absRelErrSum / runs;
            const double rmse = std::sqrt(sqErrSum / runs);
            const double mae = absErrSum / runs;
            const double rseObserved = (gtMean != 0.0) ? (stddev / gtMean) : 0.0;

            return {difference, mean, variance, bias, meanRelativeError, biasRelative, rmse, mae, stddev, rseObserved};
        }

        [[nodiscard]] size_t getNumElementiDistintiEffettivi() const noexcept { return numElementiDistintiEffettivi; }
        [[nodiscard]] const vector<uint32_t> &data() const noexcept { return valori; }
        [[nodiscard]] uint32_t getSeed() const noexcept { return seed; }

        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluateToCsv(const filesystem::path &csvPath,
                                          size_t runs,
                                          size_t sampleSize,
                                          const string &algorithmParams,
                                          double rseTheoretical,
                                          Args &&... ctorArgs) const {
            Algo algo(ctorArgs...);
            const auto stats = evaluate<Algo>(runs, sampleSize, std::forward<Args>(ctorArgs)...);
            appendCsv(csvPath, algo.getName(), algorithmParams, runs, sampleSize, rseTheoretical, stats);
            return stats;
        }

        /**
         * Viene chiamato al primo evaluate() e riutilizza i sotto-insiemi per le chiamate successive dello stesso EvaluationFramework
         * @param runs
         * @param sampleSize
         */
        void ensureSubsets(size_t runs, size_t sampleSize) const {
            // Cache: reuse only when runs and sampleSize match the cached values.
            if (!sottoInsiemi.empty()
                && cachedRuns == runs
                && cachedSampleSize == sampleSize) {
                return;
            }

            sottoInsiemi.clear();
            cachedRuns = runs;
            cachedSampleSize = sampleSize;

            sottoInsiemi.reserve(runs);
            for (size_t r = 0; r < runs; ++r) {
                vector<std::uint32_t> subset;
                subset.reserve(sampleSize);
                std::sample(valori.begin(), valori.end(),
                            std::back_inserter(subset),
                            sampleSize, rng);
                sottoInsiemi.push_back(std::move(subset));
            }
        }

    private:
        static size_t exactDistinctCount(const vector<uint32_t> &values) {
            unordered_set<uint32_t> distinct(values.begin(), values.end());
            return distinct.size();
        }

        static string escapeCsvField(const string &value) {
            const bool needsQuotes = value.find_first_of(",\"\n\r") != string::npos;
            if (!needsQuotes) return value;

            string out;
            out.reserve(value.size() + 2);
            out.push_back('"');
            for (char c : value) {
                if (c == '"') {
                    out.push_back('"');
                    out.push_back('"');
                } else {
                    out.push_back(c);
                }
            }
            out.push_back('"');
            return out;
        }

        void appendCsv(const filesystem::path &csvPath,
                       const string &algorithmName,
                       const string &algorithmParams,
                       size_t runs,
                       size_t sampleSize,
                       double rseTheoretical,
                       const Stats &stats) const {
            const bool writeHeader = !filesystem::exists(csvPath) || filesystem::file_size(csvPath) == 0;
            ofstream out(csvPath, ios::app);
            if (!out) throw runtime_error("Impossibile aprire il file CSV");

            out << std::setprecision(10);
            if (writeHeader) {
                out << "algorithm,params,runs,sample_size,dataset_size,distinct_count,seed,"
                       "mean,variance,stddev,rse_theoretical,rse_observed,bias,difference,bias_relative,"
                       "mean_relative_error,rmse,mae\n";
            }

            out << escapeCsvField(algorithmName) << ','
                << escapeCsvField(algorithmParams) << ','
                << runs << ','
                << sampleSize << ','
                << valori.size() << ','
                << numElementiDistintiEffettivi << ','
                << seed << ','
                << stats.mean << ','
                << stats.variance << ','
                << stats.stddev << ','
                << rseTheoretical << ','
                << stats.rse_observed << ','
                << stats.bias << ','
                << stats.difference << ','
                << stats.bias_relative << ','
                << stats.mean_relative_error << ','
                << stats.rmse << ','
                << stats.mae << '\n';
        }

        vector<uint32_t> valori;
        size_t numElementiDistintiEffettivi;
        uint32_t seed;
        mutable mt19937 rng;

        mutable vector<vector<uint32_t> > sottoInsiemi;
        // Cache key for generated subsets.
        mutable size_t cachedRuns = 0;
        mutable size_t cachedSampleSize = 0;
    };
} // namespace satp::evaluation
