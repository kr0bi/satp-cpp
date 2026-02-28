#include "satp/cli/AlgorithmExecutor.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <utility>

#include "satp/algorithms/HyperLogLog.h"
#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/algorithms/LogLog.h"
#include "satp/algorithms/ProbabilisticCounting.h"
#include "satp/cli/CliConfig.h"
#include "satp/cli/PathUtils.h"
#include "satp/simulation/EvaluationFramework.h"

namespace satp::cli {
    namespace {
        namespace alg = satp::algorithms;
        namespace eval = satp::evaluation;

        [[nodiscard]] bool isStreaming(const RunMode mode) {
            return mode == RunMode::Streaming;
        }

        [[nodiscard]] bool isMerge(const RunMode mode) {
            return mode == RunMode::Merge;
        }

        [[nodiscard]] const char *modeLabel(const RunMode mode) {
            if (isStreaming(mode)) return "streaming";
            if (isMerge(mode)) return "merge";
            return "normal";
        }

        [[nodiscard]] double rseHll(const std::uint32_t k) {
            const double m = static_cast<double>(1u << k);
            return 1.04 / std::sqrt(m);
        }

        [[nodiscard]] double rseLogLog(const std::uint32_t k) {
            const double m = static_cast<double>(1u << k);
            return 1.30 / std::sqrt(m);
        }

        [[nodiscard]] double rseUnknown() {
            return std::numeric_limits<double>::quiet_NaN();
        }

        [[nodiscard]] std::unordered_set<std::string> collectRequestedAlgorithms(
            const std::vector<std::string> &algs) {
            std::unordered_set<std::string> selected;
            selected.reserve(algs.size());
            for (const auto &name : algs) {
                selected.insert(name);
            }
            return selected;
        }

        [[nodiscard]] bool shouldRun(const std::unordered_set<std::string> &selected,
                                     const std::string &key) {
            return selected.contains("all") || selected.contains(key);
        }

        void printRunContext(const DatasetRuntimeContext &ctx, const RunMode mode) {
            std::cout << "mode: " << modeLabel(mode)
                      << '\t' << "sampleSize: " << ctx.sampleSize
                      << '\t' << "runs: " << ctx.runs
                      << '\t' << "seed: " << ctx.seed << '\n'
                      << "resultsRoot: " << (ctx.repoRoot / "results" / ctx.resultsNamespace).string() << '\n';
        }

        void printNormalSummary(const AlgorithmRunSpec &spec,
                                const std::filesystem::path &csvPath,
                                const eval::Stats &stats) {
            std::cout << '[' << spec.displayTag << "] csv=" << csvPath.string()
                      << "  mean=" << stats.mean
                      << "  f0_hat=" << stats.mean
                      << "  f0_true=" << stats.truth_mean
                      << "  var=" << stats.variance
                      << "  stddev=" << stats.stddev
                      << "  bias=" << stats.bias
                      << "  mre=" << stats.mean_relative_error
                      << "  rmse=" << stats.rmse
                      << "  mae=" << stats.mae << '\n';
        }

        void printStreamingSummary(const AlgorithmRunSpec &spec,
                                   const std::filesystem::path &csvPath,
                                   const eval::StreamingPointStats &lastPoint) {
            std::cout << '[' << spec.displayTag << "][stream] csv=" << csvPath.string()
                      << "  t=" << lastPoint.number_of_elements_processed
                      << "  mean=" << lastPoint.mean
                      << "  f0_hat=" << lastPoint.mean
                      << "  f0_true=" << lastPoint.truth_mean
                      << "  var=" << lastPoint.variance
                      << "  stddev=" << lastPoint.stddev
                      << "  bias=" << lastPoint.bias
                      << "  mre=" << lastPoint.mean_relative_error
                      << "  rmse=" << lastPoint.rmse
                      << "  mae=" << lastPoint.mae << '\n';
        }

        void printMergeSummary(const AlgorithmRunSpec &spec,
                               const std::filesystem::path &csvPath,
                               const eval::MergePairStats &stats) {
            std::cout << '[' << spec.displayTag << "][merge] csv=" << csvPath.string()
                      << "  pairs=" << stats.pair_count
                      << "  merge_mean=" << stats.estimate_merge_mean
                      << "  serial_mean=" << stats.estimate_serial_mean
                      << "  delta_abs_mean=" << stats.delta_merge_serial_abs_mean
                      << "  delta_abs_max=" << stats.delta_merge_serial_abs_max
                      << "  delta_rel_mean=" << stats.delta_merge_serial_rel_mean
                      << "  delta_rmse=" << stats.delta_merge_serial_rmse
                      << '\n';
        }

        template<typename Algo, typename... CtorArgs>
        void runSingleAlgorithm(eval::EvaluationFramework &bench,
                                const DatasetRuntimeContext &ctx,
                                const AlgorithmRunSpec &spec,
                                const RunMode mode,
                                CtorArgs &&... ctorArgs) {
            const std::filesystem::path csvPath = path_utils::buildResultCsvPath(
                ctx.repoRoot,
                ctx.resultsNamespace,
                spec.algorithmName,
                spec.params,
                mode);
            std::filesystem::create_directories(csvPath.parent_path());

            if (isStreaming(mode)) {
                const auto series = bench.evaluateStreamingToCsv<Algo>(
                    csvPath,
                    ctx.runs,
                    ctx.sampleSize,
                    spec.params,
                    spec.rseTheoretical,
                    std::forward<CtorArgs>(ctorArgs)...);

                if (series.empty()) {
                    std::cout << '[' << spec.displayTag << "][stream] csv=" << csvPath.string()
                              << "  no data\n";
                    return;
                }

                printStreamingSummary(spec, csvPath, series.back());
                return;
            }

            if (isMerge(mode)) {
                const auto stats = bench.evaluateMergePairsToCsv<Algo>(
                    csvPath,
                    ctx.runs,
                    ctx.sampleSize,
                    spec.params,
                    std::forward<CtorArgs>(ctorArgs)...);
                printMergeSummary(spec, csvPath, stats);
                return;
            }

            const auto stats = bench.evaluateToCsv<Algo>(
                csvPath,
                ctx.runs,
                ctx.sampleSize,
                spec.params,
                spec.rseTheoretical,
                std::forward<CtorArgs>(ctorArgs)...);
            printNormalSummary(spec, csvPath, stats);
        }
    } // namespace

    void AlgorithmExecutor::run(const RunConfig &cfg,
                                const std::vector<std::string> &algs,
                                const RunMode mode) const {
        auto ctx = config::loadDatasetRuntimeContext(cfg);
        eval::EvaluationFramework bench(std::move(ctx.index));
        const auto selected = collectRequestedAlgorithms(algs);

        printRunContext(ctx, mode);

        if (shouldRun(selected, "hllpp")) {
            const AlgorithmRunSpec spec{
                "hllpp",
                "HLL++",
                "HyperLogLog++",
                "k=" + std::to_string(cfg.k),
                rseHll(cfg.k)
            };
            runSingleAlgorithm<alg::HyperLogLogPlusPlus>(bench, ctx, spec, mode, cfg.k);
        }

        if (shouldRun(selected, "hll")) {
            const AlgorithmRunSpec spec{
                "hll",
                "HLL ",
                "HyperLogLog",
                "k=" + std::to_string(cfg.k) + ",L=" + std::to_string(cfg.lLog),
                rseHll(cfg.k)
            };
            runSingleAlgorithm<alg::HyperLogLog>(bench, ctx, spec, mode, cfg.k, cfg.lLog);
        }

        if (shouldRun(selected, "ll")) {
            const AlgorithmRunSpec spec{
                "ll",
                "LL  ",
                "LogLog",
                "k=" + std::to_string(cfg.k) + ",L=" + std::to_string(cfg.lLog),
                rseLogLog(cfg.k)
            };
            runSingleAlgorithm<alg::LogLog>(bench, ctx, spec, mode, cfg.k, cfg.lLog);
        }

        if (shouldRun(selected, "pc")) {
            const AlgorithmRunSpec spec{
                "pc",
                "PC  ",
                "ProbabilisticCounting",
                "L=" + std::to_string(cfg.l),
                rseUnknown()
            };
            runSingleAlgorithm<alg::ProbabilisticCounting>(bench, ctx, spec, mode, cfg.l);
        }
    }
} // namespace satp::cli
