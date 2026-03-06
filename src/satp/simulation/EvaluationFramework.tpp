#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "satp/ProgressBar.h"
#include "satp/simulation/CsvResultWriter.h"
#include "satp/simulation/ErrorAccumulator.h"
#include "satp/simulation/StreamingCheckpointBuilder.h"

using namespace std;

namespace satp::evaluation {
    namespace detail {
        template<typename Algo>
        concept MergeableAlgorithm = requires(Algo a, const Algo &b) {
            { a.merge(b) } -> same_as<void>;
        };

        inline MergePairStats summarizeMergePairs(const vector<MergePairPoint> &points) {
            if (points.empty()) return {};

            MergePairStats out{};
            out.pair_count = points.size();

            double mergeSum = 0.0;
            double serialSum = 0.0;
            double absSum = 0.0;
            double relSum = 0.0;
            double sqAbsSum = 0.0;
            double absMax = 0.0;
            for (const auto &point : points) {
                mergeSum += point.estimate_merge;
                serialSum += point.estimate_serial;
                absSum += point.delta_merge_serial_abs;
                relSum += point.delta_merge_serial_rel;
                sqAbsSum += point.delta_merge_serial_abs * point.delta_merge_serial_abs;
                absMax = max(absMax, point.delta_merge_serial_abs);
            }

            const double n = static_cast<double>(points.size());
            out.estimate_merge_mean = mergeSum / n;
            out.estimate_serial_mean = serialSum / n;
            out.delta_merge_serial_abs_mean = absSum / n;
            out.delta_merge_serial_abs_max = absMax;
            out.delta_merge_serial_rel_mean = relSum / n;
            out.delta_merge_serial_rmse = sqrt(sqAbsSum / n);
            return out;
        }
    } // namespace detail

    template<typename Algo, typename... Args>
    Algo EvaluationFramework::makeAlgo(Args &&... ctorArgs) const {
        static_assert(constructible_from<Algo, Args..., const hashing::HashFunction &>,
                      "Algorithm must be constructible with (..., const hashing::HashFunction&)");
        return Algo(forward<Args>(ctorArgs)..., *hashFunction);
    }

    template<typename Algo, typename... Args>
    Stats EvaluationFramework::evaluate(size_t runs,
                                        size_t sampleSize,
                                        Args &&... ctorArgs) const {
        if (runs == 0 || sampleSize == 0) return {};
        (void) runs;
        (void) sampleSize;
        return evaluateFromBinary<Algo>(forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    Stats EvaluationFramework::evaluateToCsv(const filesystem::path &csvPath,
                                             size_t runs,
                                             size_t sampleSize,
                                             const string &algorithmParams,
                                             const double rseTheoretical,
                                             Args &&... ctorArgs) const {
        (void) runs;
        (void) sampleSize;

        Algo algo = makeAlgo<Algo>(ctorArgs...);
        const auto scope = datasetScope();
        const Stats stats = evaluateFromBinary<Algo>(forward<Args>(ctorArgs)...);
        CsvResultWriter::appendNormal(
            csvPath,
            algo.getName(),
            algorithmParams,
            scope.runs,
            scope.sampleSize,
            numElementiDistintiEffettivi,
            seed,
            rseTheoretical,
            stats);
        return stats;
    }

    template<typename Algo, typename... Args>
    vector<StreamingPointStats> EvaluationFramework::evaluateStreaming(size_t runs,
                                                                            size_t sampleSize,
                                                                            Args &&... ctorArgs) const {
        if (runs == 0 || sampleSize == 0) return {};
        (void) runs;
        (void) sampleSize;
        return evaluateStreamingFromBinary<Algo>(forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    vector<StreamingPointStats> EvaluationFramework::evaluateStreamingToCsv(
        const filesystem::path &csvPath,
        size_t runs,
        size_t sampleSize,
        const string &algorithmParams,
        const double rseTheoretical,
        Args &&... ctorArgs) const {
        (void) runs;
        (void) sampleSize;

        Algo algo = makeAlgo<Algo>(ctorArgs...);
        const auto scope = datasetScope();
        auto series = evaluateStreamingFromBinary<Algo>(forward<Args>(ctorArgs)...);
        CsvResultWriter::appendStreaming(
            csvPath,
            algo.getName(),
            algorithmParams,
            scope.runs,
            scope.sampleSize,
            numElementiDistintiEffettivi,
            seed,
            rseTheoretical,
            series);
        return series;
    }

    template<typename Algo, typename... Args>
    vector<MergePairPoint> EvaluationFramework::evaluateMergePairs(size_t runs,
                                                                        size_t sampleSize,
                                                                        Args &&... ctorArgs) const {
        static_assert(detail::MergeableAlgorithm<Algo>,
                      "evaluateMergePairs requires Algo::merge(const Algo&)");

        if (runs == 0 || sampleSize == 0) return {};
        (void) runs;
        (void) sampleSize;

        const auto scope = datasetScope();
        if (scope.runs < 2 || scope.sampleSize == 0) return {};

        const size_t pairCount = scope.runs / 2u;
        satp::util::ProgressBar bar{pairCount * scope.sampleSize * 4u, cout, 50, 10'000};
        satp::io::BinaryDatasetPartitionReader reader(binaryDataset);

        vector<uint32_t> partA;
        vector<uint32_t> partB;
        vector<MergePairPoint> points;
        points.reserve(pairCount);

        for (size_t pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
            const size_t idxA = 2u * pairIndex;
            const size_t idxB = idxA + 1u;
            reader.load(idxA, partA);
            reader.load(idxB, partB);

            Algo sketchA = makeAlgo<Algo>(forward<Args>(ctorArgs)...);
            for (const auto value : partA) {
                sketchA.process(value);
                bar.tick();
            }

            Algo sketchB = makeAlgo<Algo>(forward<Args>(ctorArgs)...);
            for (const auto value : partB) {
                sketchB.process(value);
                bar.tick();
            }

            Algo merged = sketchA;
            merged.merge(sketchB);

            Algo serial = makeAlgo<Algo>(forward<Args>(ctorArgs)...);
            for (const auto value : partA) {
                serial.process(value);
                bar.tick();
            }
            for (const auto value : partB) {
                serial.process(value);
                bar.tick();
            }

            const double estimateMerge = static_cast<double>(merged.count());
            const double estimateSerial = static_cast<double>(serial.count());
            const double deltaAbs = abs(estimateMerge - estimateSerial);
            const double deltaRel = (estimateSerial != 0.0) ? (deltaAbs / estimateSerial) : 0.0;

            points.push_back({
                pairIndex,
                estimateMerge,
                estimateSerial,
                deltaAbs,
                deltaRel
            });
        }

        bar.finish();
        cout.flush();
        return points;
    }

    template<typename Algo, typename... Args>
    MergePairStats EvaluationFramework::evaluateMergePairsToCsv(
        const filesystem::path &csvPath,
        size_t runs,
        size_t sampleSize,
        const string &algorithmParams,
        Args &&... ctorArgs) const {
        Algo algo = makeAlgo<Algo>(ctorArgs...);
        const auto scope = datasetScope();
        const auto points = evaluateMergePairs<Algo>(runs, sampleSize, forward<Args>(ctorArgs)...);
        CsvResultWriter::appendMergePairs(
            csvPath,
            algo.getName(),
            algorithmParams,
            points.size(),
            scope.sampleSize,
            seed,
            points);
        return detail::summarizeMergePairs(points);
    }

    template<typename Algo, typename... Args>
    Stats EvaluationFramework::evaluateFromBinary(Args &&... ctorArgs) const {
        const auto scope = datasetScope();
        if (scope.runs == 0 || scope.sampleSize == 0) return {};

        satp::util::ProgressBar bar{scope.runs * scope.sampleSize, cout, 50, 10'000};
        satp::io::BinaryDatasetPartitionReader reader(binaryDataset);

        vector<uint32_t> partitionValues;
        ErrorAccumulator accumulator;
        const double truth = static_cast<double>(numElementiDistintiEffettivi);

        for (size_t run = 0; run < scope.runs; ++run) {
            reader.load(run, partitionValues);
            Algo algo = makeAlgo<Algo>(forward<Args>(ctorArgs)...);

            for (const auto value : partitionValues) {
                algo.process(value);
                bar.tick();
            }

            accumulator.add(static_cast<double>(algo.count()), truth);
        }

        bar.finish();
        cout.flush();
        return accumulator.toStats();
    }

    template<typename Algo, typename... Args>
    vector<StreamingPointStats> EvaluationFramework::evaluateStreamingFromBinary(Args &&... ctorArgs) const {
        const auto scope = datasetScope();
        if (scope.runs == 0 || scope.sampleSize == 0) return {};

        const auto checkpointPositions = StreamingCheckpointBuilder::build(
            scope.sampleSize,
            DEFAULT_STREAMING_CHECKPOINTS);

        satp::util::ProgressBar bar{scope.runs * scope.sampleSize, cout, 50, 10'000};
        satp::io::BinaryDatasetPartitionReader reader(binaryDataset);

        vector<ErrorAccumulator> accumulators(checkpointPositions.size());
        vector<uint32_t> partitionValues;
        vector<uint8_t> partitionTruthBits;

        for (size_t run = 0; run < scope.runs; ++run) {
            reader.loadWithTruthBits(run, partitionValues, partitionTruthBits);
            if (partitionValues.size() != scope.sampleSize) {
                throw runtime_error("Invalid binary dataset: partition size mismatch while streaming");
            }
            if (partitionTruthBits.size() != (scope.sampleSize + 7u) / 8u) {
                throw runtime_error("Invalid binary dataset: truth bitset size mismatch while streaming");
            }

            Algo algo = makeAlgo<Algo>(forward<Args>(ctorArgs)...);
            uint64_t truthPrefix = 0;
            size_t checkpointIndex = 0;

            for (size_t t = 0; t < scope.sampleSize; ++t) {
                algo.process(partitionValues[t]);

                const uint8_t byte = partitionTruthBits[t >> 3u];
                const bool isNew = ((byte >> (t & 7u)) & 0x1u) != 0;
                if (isNew) {
                    ++truthPrefix;
                }

                const size_t elementIndex = t + 1u;
                if (checkpointIndex < checkpointPositions.size()
                    && elementIndex == checkpointPositions[checkpointIndex]) {
                    accumulators[checkpointIndex].add(
                        static_cast<double>(algo.count()),
                        static_cast<double>(truthPrefix));
                    ++checkpointIndex;
                }

                bar.tick();
            }
        }

        bar.finish();
        cout.flush();

        vector<StreamingPointStats> out;
        out.reserve(checkpointPositions.size());
        for (size_t i = 0; i < checkpointPositions.size(); ++i) {
            out.push_back(accumulators[i].toStreamingPoint(checkpointPositions[i]));
        }
        return out;
    }
} // namespace satp::evaluation
