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
#include <optional>
#include <limits>

#include "../algorithms/Algorithm.h"
#include "satp/ProgressBar.h"
#include "satp/io/BinaryDatasetIO.h"

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
        double truth_mean = 0.0; // \bar{F}_0
    };

    struct StreamingPointStats {
        size_t element_index = 0; // 1-based index t
        double difference = 0.0;
        double mean = 0.0; // \bar{\hat{F}_0(t)}
        double variance = 0.0;
        double bias = 0.0;
        double mean_relative_error = 0.0;
        double bias_relative = 0.0;
        double rmse = 0.0;
        double mae = 0.0;
        double stddev = 0.0;
        double rse_observed = 0.0;
        double truth_mean = 0.0; // \bar{F_0(t)}
    };

    class EvaluationFramework {
    public:
        static constexpr uint32_t DEFAULT_SEED = 5489u;

        explicit EvaluationFramework(const filesystem::path &filePath)
            : seed(DEFAULT_SEED),
              rng(seed) {
            binaryDataset = satp::io::indexBinaryDataset(filePath);
            numElementiDistintiEffettivi = binaryDataset->info.distinct_per_partition;
            seed = binaryDataset->info.seed;
            rng.seed(seed);
        }

        explicit EvaluationFramework(satp::io::BinaryDatasetIndex datasetIndex)
            : seed(DEFAULT_SEED),
              rng(seed) {
            binaryDataset = std::move(datasetIndex);
            numElementiDistintiEffettivi = binaryDataset->info.distinct_per_partition;
            seed = binaryDataset->info.seed;
            rng.seed(seed);
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

            if (binaryDataset.has_value()) {
                (void) runs;
                (void) sampleSize;
                return evaluateFromBinary<Algo>(std::forward<Args>(ctorArgs)...);
            }

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

            return {difference, mean, variance, bias, meanRelativeError, biasRelative, rmse, mae, stddev, rseObserved, gtMean};
        }

        [[nodiscard]] size_t getNumElementiDistintiEffettivi() const noexcept { return numElementiDistintiEffettivi; }
        [[nodiscard]] const vector<uint32_t> &data() const noexcept { return valori; }
        [[nodiscard]] uint32_t getSeed() const noexcept { return seed; }
        [[nodiscard]] size_t getDatasetRuns() const noexcept {
            return binaryDataset.has_value() ? binaryDataset->info.partition_count : 0;
        }
        [[nodiscard]] size_t getDatasetSampleSize() const noexcept {
            return binaryDataset.has_value() ? binaryDataset->info.elements_per_partition : 0;
        }

        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluateToCsv(const filesystem::path &csvPath,
                                          size_t runs,
                                          size_t sampleSize,
                                          const string &algorithmParams,
                                          double rseTheoretical,
                                          Args &&... ctorArgs) const {
            Algo algo(ctorArgs...);
            const size_t effectiveRuns = binaryDataset.has_value() ? binaryDataset->info.partition_count : runs;
            const size_t effectiveSampleSize = binaryDataset.has_value() ? binaryDataset->info.elements_per_partition : sampleSize;
            const auto stats = evaluate<Algo>(effectiveRuns, effectiveSampleSize, std::forward<Args>(ctorArgs)...);
            appendCsv(csvPath, algo.getName(), algorithmParams, effectiveRuns, effectiveSampleSize, rseTheoretical, stats);
            return stats;
        }

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<StreamingPointStats> evaluateStreaming(size_t runs,
                                                                    size_t sampleSize,
                                                                    Args &&... ctorArgs) const {
            if (!binaryDataset.has_value()) {
                throw runtime_error("Streaming evaluation is supported only for binary datasets");
            }
            (void) runs;
            (void) sampleSize;
            return evaluateStreamingFromBinary<Algo>(std::forward<Args>(ctorArgs)...);
        }

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<StreamingPointStats> evaluateStreamingToCsv(const filesystem::path &csvPath,
                                                                         size_t runs,
                                                                         size_t sampleSize,
                                                                         const string &algorithmParams,
                                                                         double rseTheoretical,
                                                                         Args &&... ctorArgs) const {
            Algo algo(ctorArgs...);
            const size_t effectiveRuns = binaryDataset.has_value() ? binaryDataset->info.partition_count : runs;
            const size_t effectiveSampleSize = binaryDataset.has_value() ? binaryDataset->info.elements_per_partition : sampleSize;
            auto series = evaluateStreaming<Algo>(effectiveRuns, effectiveSampleSize, std::forward<Args>(ctorArgs)...);
            appendStreamingCsv(csvPath, algo.getName(), algorithmParams, effectiveRuns, effectiveSampleSize, rseTheoretical, series);
            return series;
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
        struct StreamingAccumulator {
            double estimateMean = 0.0;
            double estimateM2 = 0.0;
            double truthSum = 0.0;
            double absErrSum = 0.0;
            double sqErrSum = 0.0;
            double absRelErrSum = 0.0;
            size_t count = 0;
        };

        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluateFromBinary(Args &&... ctorArgs) const {
            if (!binaryDataset.has_value()) {
                throw runtime_error("Internal error: binary dataset is not loaded");
            }

            const auto &info = binaryDataset->info;
            const size_t runs = info.partition_count;
            const size_t sampleSize = info.elements_per_partition;

            using satp::util::ProgressBar;
            ProgressBar bar{runs * sampleSize, cout, 50, 10'000};

            vector<double> estimates;
            estimates.reserve(runs);

            vector<double> truths;
            truths.reserve(runs);

            double absErrSum = 0.0;
            double sqErrSum = 0.0;
            double absRelErrSum = 0.0;

            satp::io::BinaryDatasetPartitionReader reader(*binaryDataset);
            vector<uint32_t> partitionValues;
            for (size_t r = 0; r < runs; ++r) {
                reader.load(r, partitionValues);
                Algo algo(std::forward<Args>(ctorArgs)...);

                for (auto v: partitionValues) {
                    algo.process(v);
                    bar.tick();
                }

                const double estimate = static_cast<double>(algo.count());
                const double truth = static_cast<double>(numElementiDistintiEffettivi);
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

            return {difference, mean, variance, bias, meanRelativeError, biasRelative, rmse, mae, stddev, rseObserved, gtMean};
        }

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<StreamingPointStats> evaluateStreamingFromBinary(Args &&... ctorArgs) const {
            if (!binaryDataset.has_value()) {
                throw runtime_error("Internal error: binary dataset is not loaded");
            }

            const auto &info = binaryDataset->info;
            const size_t runs = info.partition_count;
            const size_t sampleSize = info.elements_per_partition;
            if (runs == 0 || sampleSize == 0) return {};

            using satp::util::ProgressBar;
            ProgressBar bar{runs * sampleSize, cout, 50, 10'000};

            vector<StreamingAccumulator> accumulators(sampleSize);

            satp::io::BinaryDatasetPartitionReader reader(*binaryDataset);
            vector<uint32_t> partitionValues;
            vector<uint8_t> partitionTruthBits;

            for (size_t r = 0; r < runs; ++r) {
                reader.loadWithTruthBits(r, partitionValues, partitionTruthBits);
                if (partitionValues.size() != sampleSize) {
                    throw runtime_error("Invalid binary dataset: partition size mismatch while streaming");
                }
                if (partitionTruthBits.size() != (sampleSize + 7u) / 8u) {
                    throw runtime_error("Invalid binary dataset: truth bitset size mismatch while streaming");
                }

                Algo algo(std::forward<Args>(ctorArgs)...);
                std::uint64_t truthPrefix = 0;

                for (size_t t = 0; t < sampleSize; ++t) {
                    const std::uint32_t value = partitionValues[t];
                    algo.process(value);

                    const std::uint8_t byte = partitionTruthBits[t >> 3u];
                    const bool isNew = ((byte >> (t & 7u)) & 0x1u) != 0;
                    if (isNew) {
                        ++truthPrefix;
                    }

                    const double estimate = static_cast<double>(algo.count());
                    const double truth = static_cast<double>(truthPrefix);
                    const double err = estimate - truth;

                    auto &acc = accumulators[t];
                    ++acc.count;
                    const double delta = estimate - acc.estimateMean;
                    acc.estimateMean += delta / static_cast<double>(acc.count);
                    const double delta2 = estimate - acc.estimateMean;
                    acc.estimateM2 += delta * delta2;

                    acc.truthSum += truth;
                    acc.absErrSum += std::abs(err);
                    acc.sqErrSum += err * err;
                    if (truth > 0.0) {
                        acc.absRelErrSum += std::abs(err) / truth;
                    }

                    bar.tick();
                }
            }

            bar.finish();
            cout.flush();

            vector<StreamingPointStats> out;
            out.reserve(sampleSize);
            for (size_t t = 0; t < sampleSize; ++t) {
                const auto &acc = accumulators[t];
                const double runCount = static_cast<double>(acc.count);
                const double mean = acc.estimateMean;
                const double gtMean = acc.truthSum / runCount;
                const double variance = (acc.count > 1) ? (acc.estimateM2 / static_cast<double>(acc.count - 1)) : 0.0;
                const double stddev = std::sqrt(variance);
                const double bias = mean - gtMean;
                const double difference = std::abs(bias);
                const double biasRelative = (gtMean != 0.0) ? (bias / gtMean) : 0.0;
                const double meanRelativeError = acc.absRelErrSum / runCount;
                const double rmse = std::sqrt(acc.sqErrSum / runCount);
                const double mae = acc.absErrSum / runCount;
                const double rseObserved = (gtMean != 0.0) ? (stddev / gtMean) : 0.0;

                out.push_back({
                    t + 1,
                    difference,
                    mean,
                    variance,
                    bias,
                    meanRelativeError,
                    biasRelative,
                    rmse,
                    mae,
                    stddev,
                    rseObserved,
                    gtMean
                });
            }

            return out;
        }

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

        static void writeCsvHeaderIfNeeded(const filesystem::path &csvPath, std::ofstream &out) {
            const bool writeHeader = !filesystem::exists(csvPath) || filesystem::file_size(csvPath) == 0;
            if (!writeHeader) return;

            out << "algorithm,params,mode,runs,sample_size,element_index,distinct_count,seed,"
                   "f0_mean,f0_hat_mean,"
                   "mean,variance,stddev,rse_theoretical,rse_observed,bias,difference,bias_relative,"
                   "mean_relative_error,rmse,mae\n";
        }

        void appendCsv(const filesystem::path &csvPath,
                       const string &algorithmName,
                       const string &algorithmParams,
                       size_t runs,
                       size_t sampleSize,
                       double rseTheoretical,
                       const Stats &stats) const {
            ofstream out(csvPath, ios::app);
            if (!out) throw runtime_error("Impossibile aprire il file CSV");

            out << std::setprecision(10);
            writeCsvHeaderIfNeeded(csvPath, out);

            out << escapeCsvField(algorithmName) << ','
                << escapeCsvField(algorithmParams) << ','
                << "normal,"
                << runs << ','
                << sampleSize << ','
                << sampleSize << ','
                << numElementiDistintiEffettivi << ','
                << seed << ','
                << stats.truth_mean << ','
                << stats.mean << ','
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

        void appendStreamingCsv(const filesystem::path &csvPath,
                                const string &algorithmName,
                                const string &algorithmParams,
                                size_t runs,
                                size_t sampleSize,
                                double rseTheoretical,
                                const vector<StreamingPointStats> &series) const {
            ofstream out(csvPath, ios::app);
            if (!out) throw runtime_error("Impossibile aprire il file CSV");

            out << std::setprecision(10);
            writeCsvHeaderIfNeeded(csvPath, out);

            for (const auto &point : series) {
                out << escapeCsvField(algorithmName) << ','
                    << escapeCsvField(algorithmParams) << ','
                    << "streaming,"
                    << runs << ','
                    << sampleSize << ','
                    << point.element_index << ','
                    << numElementiDistintiEffettivi << ','
                    << seed << ','
                    << point.truth_mean << ','
                    << point.mean << ','
                    << point.mean << ','
                    << point.variance << ','
                    << point.stddev << ','
                    << rseTheoretical << ','
                    << point.rse_observed << ','
                    << point.bias << ','
                    << point.difference << ','
                    << point.bias_relative << ','
                    << point.mean_relative_error << ','
                    << point.rmse << ','
                    << point.mae << '\n';
            }
        }

        vector<uint32_t> valori;
        size_t numElementiDistintiEffettivi;
        uint32_t seed;
        mutable mt19937 rng;

        mutable vector<vector<uint32_t> > sottoInsiemi;
        // Cache key for generated subsets.
        mutable size_t cachedRuns = 0;
        mutable size_t cachedSampleSize = 0;

        std::optional<satp::io::BinaryDatasetIndex> binaryDataset;
    };
} // namespace satp::evaluation
