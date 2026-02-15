#pragma once

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
