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

namespace satp::evaluation {
    namespace detail {
        template<typename Algo>
        concept MergeableAlgorithm = requires(Algo a, const Algo &b) {
            { a.merge(b) } -> std::same_as<void>;
        };

        inline MergePairStats summarizeMergePairs(const std::vector<MergePairPoint> &points) {
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
                absMax = std::max(absMax, point.delta_merge_serial_abs);
            }

            const double n = static_cast<double>(points.size());
            out.estimate_merge_mean = mergeSum / n;
            out.estimate_serial_mean = serialSum / n;
            out.delta_merge_serial_abs_mean = absSum / n;
            out.delta_merge_serial_abs_max = absMax;
            out.delta_merge_serial_rel_mean = relSum / n;
            out.delta_merge_serial_rmse = std::sqrt(sqAbsSum / n);
            return out;
        }
    } // namespace detail

    template<typename Algo, typename... Args>
    Stats EvaluationFramework::evaluate(std::size_t runs,
                                        std::size_t sampleSize,
                                        Args &&... ctorArgs) const {
        if (runs == 0 || sampleSize == 0) return {};
        (void) runs;
        (void) sampleSize;
        return evaluateFromBinary<Algo>(std::forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    Stats EvaluationFramework::evaluateToCsv(const std::filesystem::path &csvPath,
                                             std::size_t runs,
                                             std::size_t sampleSize,
                                             const std::string &algorithmParams,
                                             const double rseTheoretical,
                                             Args &&... ctorArgs) const {
        (void) runs;
        (void) sampleSize;

        Algo algo(ctorArgs...);
        const auto scope = datasetScope();
        const Stats stats = evaluateFromBinary<Algo>(std::forward<Args>(ctorArgs)...);
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
    std::vector<StreamingPointStats> EvaluationFramework::evaluateStreaming(std::size_t runs,
                                                                            std::size_t sampleSize,
                                                                            Args &&... ctorArgs) const {
        if (runs == 0 || sampleSize == 0) return {};
        (void) runs;
        (void) sampleSize;
        return evaluateStreamingFromBinary<Algo>(std::forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    std::vector<StreamingPointStats> EvaluationFramework::evaluateStreamingToCsv(
        const std::filesystem::path &csvPath,
        std::size_t runs,
        std::size_t sampleSize,
        const std::string &algorithmParams,
        const double rseTheoretical,
        Args &&... ctorArgs) const {
        (void) runs;
        (void) sampleSize;

        Algo algo(ctorArgs...);
        const auto scope = datasetScope();
        auto series = evaluateStreamingFromBinary<Algo>(std::forward<Args>(ctorArgs)...);
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
    std::vector<MergePairPoint> EvaluationFramework::evaluateMergePairs(std::size_t runs,
                                                                        std::size_t sampleSize,
                                                                        Args &&... ctorArgs) const {
        static_assert(detail::MergeableAlgorithm<Algo>,
                      "evaluateMergePairs requires Algo::merge(const Algo&)");

        if (runs == 0 || sampleSize == 0) return {};
        (void) runs;
        (void) sampleSize;

        const auto scope = datasetScope();
        if (scope.runs < 2 || scope.sampleSize == 0) return {};

        const std::size_t pairCount = scope.runs / 2u;
        satp::util::ProgressBar bar{pairCount * scope.sampleSize * 4u, std::cout, 50, 10'000};
        satp::io::BinaryDatasetPartitionReader reader(binaryDataset);

        std::vector<std::uint32_t> partA;
        std::vector<std::uint32_t> partB;
        std::vector<MergePairPoint> points;
        points.reserve(pairCount);

        for (std::size_t pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
            const std::size_t idxA = 2u * pairIndex;
            const std::size_t idxB = idxA + 1u;
            reader.load(idxA, partA);
            reader.load(idxB, partB);

            Algo sketchA(std::forward<Args>(ctorArgs)...);
            for (const auto value : partA) {
                sketchA.process(value);
                bar.tick();
            }

            Algo sketchB(std::forward<Args>(ctorArgs)...);
            for (const auto value : partB) {
                sketchB.process(value);
                bar.tick();
            }

            Algo merged = sketchA;
            merged.merge(sketchB);

            Algo serial(std::forward<Args>(ctorArgs)...);
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
            const double deltaAbs = std::abs(estimateMerge - estimateSerial);
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
        std::cout.flush();
        return points;
    }

    template<typename Algo, typename... Args>
    MergePairStats EvaluationFramework::evaluateMergePairsToCsv(
        const std::filesystem::path &csvPath,
        std::size_t runs,
        std::size_t sampleSize,
        const std::string &algorithmParams,
        Args &&... ctorArgs) const {
        Algo algo(ctorArgs...);
        const auto scope = datasetScope();
        const auto points = evaluateMergePairs<Algo>(runs, sampleSize, std::forward<Args>(ctorArgs)...);
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

        satp::util::ProgressBar bar{scope.runs * scope.sampleSize, std::cout, 50, 10'000};
        satp::io::BinaryDatasetPartitionReader reader(binaryDataset);

        std::vector<std::uint32_t> partitionValues;
        ErrorAccumulator accumulator;
        const double truth = static_cast<double>(numElementiDistintiEffettivi);

        for (std::size_t run = 0; run < scope.runs; ++run) {
            reader.load(run, partitionValues);
            Algo algo(std::forward<Args>(ctorArgs)...);

            for (const auto value : partitionValues) {
                algo.process(value);
                bar.tick();
            }

            accumulator.add(static_cast<double>(algo.count()), truth);
        }

        bar.finish();
        std::cout.flush();
        return accumulator.toStats();
    }

    template<typename Algo, typename... Args>
    std::vector<StreamingPointStats> EvaluationFramework::evaluateStreamingFromBinary(Args &&... ctorArgs) const {
        const auto scope = datasetScope();
        if (scope.runs == 0 || scope.sampleSize == 0) return {};

        const auto checkpointPositions = StreamingCheckpointBuilder::build(
            scope.sampleSize,
            DEFAULT_STREAMING_CHECKPOINTS);

        satp::util::ProgressBar bar{scope.runs * scope.sampleSize, std::cout, 50, 10'000};
        satp::io::BinaryDatasetPartitionReader reader(binaryDataset);

        std::vector<ErrorAccumulator> accumulators(checkpointPositions.size());
        std::vector<std::uint32_t> partitionValues;
        std::vector<std::uint8_t> partitionTruthBits;

        for (std::size_t run = 0; run < scope.runs; ++run) {
            reader.loadWithTruthBits(run, partitionValues, partitionTruthBits);
            if (partitionValues.size() != scope.sampleSize) {
                throw std::runtime_error("Invalid binary dataset: partition size mismatch while streaming");
            }
            if (partitionTruthBits.size() != (scope.sampleSize + 7u) / 8u) {
                throw std::runtime_error("Invalid binary dataset: truth bitset size mismatch while streaming");
            }

            Algo algo(std::forward<Args>(ctorArgs)...);
            std::uint64_t truthPrefix = 0;
            std::size_t checkpointIndex = 0;

            for (std::size_t t = 0; t < scope.sampleSize; ++t) {
                algo.process(partitionValues[t]);

                const std::uint8_t byte = partitionTruthBits[t >> 3u];
                const bool isNew = ((byte >> (t & 7u)) & 0x1u) != 0;
                if (isNew) {
                    ++truthPrefix;
                }

                const std::size_t elementIndex = t + 1u;
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
        std::cout.flush();

        std::vector<StreamingPointStats> out;
        out.reserve(checkpointPositions.size());
        for (std::size_t i = 0; i < checkpointPositions.size(); ++i) {
            out.push_back(accumulators[i].toStreamingPoint(checkpointPositions[i]));
        }
        return out;
    }
} // namespace satp::evaluation
