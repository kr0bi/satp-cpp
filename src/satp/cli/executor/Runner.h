#pragma once

#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "satp/cli/CliTypes.h"
#include "satp/cli/PathUtils.h"
#include "satp/cli/executor/Output.h"
#include "satp/simulation/EvaluationFramework.h"

using namespace std;

namespace satp::cli::executor {
    struct AlgorithmJob {
        AlgorithmRunSpec spec;
        function<void(const AlgorithmRunSpec &)> run;
    };

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

        if (mode == RunMode::Streaming) {
            const auto series = bench.evaluateStreamingToCsv<Algo>(
                csvPath,
                ctx.runs,
                ctx.sampleSize,
                spec.params,
                spec.rseTheoretical,
                forward<CtorArgs>(ctorArgs)...);

            if (series.empty()) {
                cout << algorithmLogPrefix(spec) << "[stream] csv=" << csvPath.string()
                          << "  no data\n";
                return;
            }

            printStreamingSummary(spec, csvPath, series.back());
            return;
        }

        if (mode == RunMode::Merge) {
            const auto stats = bench.evaluateMergePairsToCsv<Algo>(
                csvPath,
                ctx.runs,
                ctx.sampleSize,
                spec.params,
                forward<CtorArgs>(ctorArgs)...);
            printMergeSummary(spec, csvPath, stats);
            return;
        }

        const auto stats = bench.evaluateToCsv<Algo>(
            csvPath,
            ctx.runs,
            ctx.sampleSize,
            spec.params,
            spec.rseTheoretical,
            forward<CtorArgs>(ctorArgs)...);
        printNormalSummary(spec, csvPath, stats);
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
                move(algorithmId),
                move(params),
                move(hashName),
                rseTheoretical
            },
            [&bench, &ctx, mode, ... capturedArgs = forward<CtorArgs>(ctorArgs)](
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
