#pragma once

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

#include "../Utils.h"
#include "../algorithms/Algorithm.h"
#include "satp/ProgressBar.h"
#include "satp/io/BinaryIO.h"

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
    };

    class EvaluationFramework {
    public:
        static constexpr uint32_t DEFAULT_SEED = 5489u;

        static Dataset makeRandomDataset(size_t numberOfElements,
                                         size_t highestNumber,
                                         uint32_t seed = DEFAULT_SEED) {
            Dataset dataset;
            dataset.values.resize(numberOfElements);

            mt19937 rng(seed);
            uniform_int_distribution<uint32_t> dist(0u, static_cast<uint32_t>(highestNumber));

            unordered_set<uint32_t> distinct;
            distinct.reserve(numberOfElements);

            for (size_t i = 0; i < numberOfElements; ++i) {
                const auto value = dist(rng);
                dataset.values[i] = value;
                distinct.insert(value);
            }

            dataset.distinct_count = distinct.size();
            return dataset;
        }

        explicit EvaluationFramework(Dataset dataset,
                                     uint32_t seed = DEFAULT_SEED)
            : valori(move(dataset.values)),
              numElementiDistintiEffettivi(dataset.distinct_count),
              seed(seed),
              rng(seed) {
        }

        explicit EvaluationFramework(vector<uint32_t> data,
                                     uint32_t seed = DEFAULT_SEED)
            : valori(move(data)),
              numElementiDistintiEffettivi(satp::utils::count_distinct(valori)),
              seed(seed),
              rng(seed) {
        }

        EvaluationFramework(const filesystem::path &filePath,
                            size_t runs,
                            size_t sampleSize,
                            size_t numberOfElements,
                            size_t highestNumber,
                            uint32_t seed = DEFAULT_SEED)
            : seed(seed),
              rng(seed) {
            try {
                satp::io::loadDataset(filePath, valori, sottoInsiemi);
                if (sottoInsiemi.size() != runs || (!sottoInsiemi.empty() && sottoInsiemi[0].size() != sampleSize))
                    throw runtime_error("Cached file has wrong shape");
                numElementiDistintiEffettivi = satp::utils::count_distinct(valori);
                cout << "[cache] dataset e subset caricati da: " << filePath << '\n';
            } catch (...) {
                cout << "[cache] assente o incompatibile, genero ex-novo\n";
                auto dataset = makeRandomDataset(numberOfElements, highestNumber, seed);
                valori = std::move(dataset.values);
                numElementiDistintiEffettivi = dataset.distinct_count;
                ensureSubsets(runs, sampleSize); // genera subset
                satp::io::saveDataset(filePath, valori, sottoInsiemi);
                cout << "[cache] salvato in: " << filePath << '\n';
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

            double absErrSum = 0.0;
            double sqErrSum = 0.0;
            double absRelErrSum = 0.0;

            for (size_t r = 0; r < runs; ++r) {
                const auto &sottoInsieme = sottoInsiemi[r];
                Algo algo(forward<Args>(ctorArgs)...);

                for (auto v: sottoInsieme) {
                    algo.process(v);
                    bar.tick();
                }
                const double estimate = static_cast<double>(algo.count());
                const double truth = static_cast<double>(utils::count_distinct(sottoInsieme));
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

            return {difference, mean, variance, bias, meanRelativeError, biasRelative, rmse, mae, stddev};
        }

        [[nodiscard]] size_t getNumElementiDistintiEffettivi() const noexcept { return numElementiDistintiEffettivi; }
        [[nodiscard]] const vector<uint32_t> &data() const noexcept { return valori; }
        [[nodiscard]] uint32_t getSeed() const noexcept { return seed; }

        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluateToCsv(const filesystem::path &csvPath,
                                          size_t runs,
                                          size_t sampleSize,
                                          const string &algorithmParams,
                                          Args &&... ctorArgs) const {
            Algo algo(ctorArgs...);
            const auto stats = evaluate<Algo>(runs, sampleSize, forward<Args>(ctorArgs)...);
            appendCsv(csvPath, algo.getName(), algorithmParams, runs, sampleSize, stats);
            return stats;
        }

        /**
         * Viene chiamato al primo evaluate() e riutilizza i sotto-insiemi per le chiamate successive dello stesso EvaluationFramework
         * @param runs
         * @param sampleSize
         */
        void ensureSubsets(size_t runs, size_t sampleSize) const {
            if (!sottoInsiemi.empty()) return;

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
                       const Stats &stats) const {
            const bool writeHeader = !filesystem::exists(csvPath) || filesystem::file_size(csvPath) == 0;
            ofstream out(csvPath, ios::app);
            if (!out) throw runtime_error("Impossibile aprire il file CSV");

            out << std::setprecision(10);
            if (writeHeader) {
                out << "algorithm,params,runs,sample_size,dataset_size,distinct_count,seed,"
                       "mean,variance,stddev,bias,difference,bias_relative,mean_relative_error,rmse,mae\n";
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
        mutable size_t cachedRuns = 0;
        mutable size_t cachedSampleSize = 0;
    };
} // namespace satp::evaluation
