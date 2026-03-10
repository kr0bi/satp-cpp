#pragma once

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "satp/algorithms/AlgorithmCatalog.h"
#include "satp/cli/detail/CliTypes.h"
#include "satp/cli/detail/execution/ProgressReporter.h"
#include "satp/cli/detail/execution/RunReporter.h"
#include "satp/cli/detail/paths/ResultPaths.h"
#include "satp/simulation/Simulation.h"

using namespace std;

namespace satp::cli::executor {
    struct AlgorithmJob {
        AlgorithmRunSpec spec;
        function<void(const AlgorithmRunSpec &)> run;
    };

    [[nodiscard]] inline satp::evaluation::CsvRunDescriptor makeCsvRunDescriptor(
        const AlgorithmRunSpec &spec,
        const satp::evaluation::EvaluationMetadata &metadata) {
        return {
            satp::algorithms::catalog::getNameBy(spec.algorithmId),
            spec.params,
            metadata,
            spec.rseTheoretical
        };
    }

    template<typename Getter>
    [[nodiscard]] inline double finiteMean(const vector<satp::evaluation::HeterogeneousMergePoint> &points,
                                           Getter getter) {
        double sum = 0.0;
        size_t count = 0;
        for (const auto &point : points) {
            const double value = getter(point);
            if (!isfinite(value)) continue;
            sum += value;
            ++count;
        }
        if (count == 0u) {
            return numeric_limits<double>::quiet_NaN();
        }
        return sum / static_cast<double>(count);
    }

    template<typename Algo, typename... CtorArgs>
    void runSingleAlgorithm(satp::evaluation::EvaluationFramework &bench,
                            const DatasetRuntimeContext &ctx,
                            const AlgorithmRunSpec &spec,
                            const RunMode mode,
                            CtorArgs &&... ctorArgs) {
        const filesystem::path csvPath = path_utils::buildResultCsvPath(
            ctx.repoRoot,
            ctx.resultsNamespace,
            spec.algorithmId,
            spec.params,
            spec.hashName,
            mode);
        filesystem::create_directories(csvPath.parent_path());
        const auto descriptor = makeCsvRunDescriptor(spec, bench.metadata());
        ProgressReporter progressReporter;
        const auto progress = progressReporter.callbacks();
        if (mode == RunMode::Streaming) {
            const auto series = bench.evaluateStreaming<Algo>(progress, std::forward<CtorArgs>(ctorArgs)...);
            satp::evaluation::CsvResultWriter::appendStreaming(csvPath, descriptor, series);
            if (series.empty()) {
                cout << algorithmLogPrefix(spec) << "[stream] csv=" << csvPath.string()
                          << "  no data\n";
                return;
            }
            printStreamingSummary(spec, csvPath, series.back());
            return;
        }
        const auto points = bench.evaluateMergePairs<Algo>(progress, std::forward<CtorArgs>(ctorArgs)...);
        satp::evaluation::CsvResultWriter::appendMergePairs(csvPath, descriptor, points);
        const auto stats = satp::evaluation::summarizeMergePairs(points);
        printMergeSummary(spec, csvPath, stats);
    }

    template<typename Algo, typename Builder>
    void runSingleHeterogeneousAlgorithm(
        satp::evaluation::EvaluationFramework &bench,
        const DatasetRuntimeContext &ctx,
        const AlgorithmRunSpec &spec,
        const satp::evaluation::HeterogeneousMergeRunDescriptor &descriptor,
        Builder buildAlgo) {
        const filesystem::path csvPath = path_utils::buildResultCsvPath(
            ctx.repoRoot,
            ctx.resultsNamespace,
            spec.algorithmId,
            spec.params,
            spec.hashName,
            RunMode::MergeHeterogeneous);
        filesystem::create_directories(csvPath.parent_path());

        ProgressReporter progressReporter;
        const auto progress = progressReporter.callbacks();
        const auto points = bench.evaluateHeterogeneousMergePairs<Algo>(descriptor, progress, buildAlgo);
        satp::evaluation::CsvResultWriter::appendHeterogeneousMergePairs(csvPath, descriptor, points);

        const size_t finiteMergePairs = static_cast<size_t>(count_if(
            points.begin(),
            points.end(),
            [](const satp::evaluation::HeterogeneousMergePoint &point) {
                return isfinite(point.estimate_merge);
            }));
        cout << algorithmLogPrefix(spec) << "[merge_heterogeneous] csv=" << csvPath.string()
                  << "  pairs=" << points.size()
                  << "  finite_merge_pairs=" << finiteMergePairs
                  << "  exact_mean=" << finiteMean(points, [](const auto &point) { return point.exact_union; })
                  << "  merge_mean=" << finiteMean(points, [](const auto &point) { return point.estimate_merge; })
                  << "  serial_mean=" << finiteMean(points, [](const auto &point) { return point.estimate_serial; })
                  << "  merge_rel_exact_mean="
                  << finiteMean(points, [](const auto &point) { return point.error_merge_rel_exact; })
                  << "  serial_rel_exact_mean="
                  << finiteMean(points, [](const auto &point) { return point.error_serial_rel_exact; })
                  << "  validity=" << satp::evaluation::toString(descriptor.validity)
                  << "  strategy=" << satp::evaluation::toString(descriptor.strategy)
                  << '\n';
    }

    template<typename Algo, typename... CtorArgs>
    void addAlgorithmJob(vector<AlgorithmJob> &jobs,
                         satp::evaluation::EvaluationFramework &bench,
                         const DatasetRuntimeContext &ctx,
                         const RunMode mode,
                         string algorithmId,
                         string params,
                         string hashName,
                         const double rseTheoretical,
                         CtorArgs &&... ctorArgs) {
        jobs.push_back({
            {
                std::move(algorithmId),
                std::move(params),
                std::move(hashName),
                rseTheoretical
            },
            [&bench, &ctx, mode, ... capturedArgs = std::forward<CtorArgs>(ctorArgs)](
                const AlgorithmRunSpec &spec) {
                runSingleAlgorithm<Algo>(
                    bench,
                    ctx,
                    spec,
                    mode,
                    capturedArgs...);
            }
                });
    }

    template<typename Algo, typename Builder>
    void addHeterogeneousAlgorithmJob(
        vector<AlgorithmJob> &jobs,
        satp::evaluation::EvaluationFramework &bench,
        const DatasetRuntimeContext &ctx,
        string algorithmId,
        string params,
        string hashName,
        const satp::evaluation::HeterogeneousMergeRunDescriptor &descriptor,
        Builder buildAlgo) {
        jobs.push_back({
            {
                std::move(algorithmId),
                std::move(params),
                std::move(hashName),
                rseUnknown()
            },
            [&bench, &ctx, descriptor, buildAlgo](const AlgorithmRunSpec &spec) {
                runSingleHeterogeneousAlgorithm<Algo>(
                    bench,
                    ctx,
                    spec,
                    descriptor,
                    buildAlgo);
            }
        });
    }
} // namespace satp::cli::executor
