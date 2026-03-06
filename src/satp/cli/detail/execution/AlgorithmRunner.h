#pragma once

#include <filesystem>
#include <functional>
#include <iostream>
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
} // namespace satp::cli::executor
