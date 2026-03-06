#include "satp/cli/AlgorithmExecutor.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <utility>

#include "satp/algorithms/AlgorithmCatalog.h"
#include "satp/algorithms/HyperLogLog.h"
#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/algorithms/LogLog.h"
#include "satp/algorithms/ProbabilisticCounting.h"
#include "satp/cli/CliConfig.h"
#include "satp/cli/PathUtils.h"
#include "satp/hashing/HashFactory.h"
#include "satp/simulation/EvaluationFramework.h"

using namespace std;

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

        [[nodiscard]] double rseHll(const uint32_t k) {
            const double m = static_cast<double>(1u << k);
            return 1.04 / sqrt(m);
        }

        [[nodiscard]] double rseLogLog(const uint32_t k) {
            const double m = static_cast<double>(1u << k);
            return 1.30 / sqrt(m);
        }

        [[nodiscard]] double rseUnknown() {
            return numeric_limits<double>::quiet_NaN();
        }

        [[nodiscard]] unordered_set<string> collectRequestedAlgorithms(
            const vector<string> &algs) {
            unordered_set<string> selected;
            selected.reserve(algs.size());
            for (const auto &name : algs) {
                selected.insert(name);
            }
            return selected;
        }

        [[nodiscard]] bool shouldRun(const unordered_set<string> &selected,
                                     const string &key) {
            return selected.contains("all") || selected.contains(key);
        }

        [[nodiscard]] string algorithmLogPrefix(const AlgorithmRunSpec &spec) {
            return "[" + spec.algorithmId + "|" + satp::algorithms::catalog::getNameBy(spec.algorithmId) + "]";
        }

        void printRunContext(const DatasetRuntimeContext &ctx, const RunMode mode, const string &hashName) {
            cout << "mode: " << modeLabel(mode)
                      << '\t' << "sampleSize: " << ctx.sampleSize
                      << '\t' << "runs: " << ctx.runs
                      << '\t' << "seed: " << ctx.seed
                      << '\t' << "hash: " << hashName << '\n'
                      << "resultsRoot: " << (ctx.repoRoot / "results" / ctx.resultsNamespace).string() << '\n';
        }

        void printNormalSummary(const AlgorithmRunSpec &spec,
                                const filesystem::path &csvPath,
                                const eval::Stats &stats) {
            cout << algorithmLogPrefix(spec) << " csv=" << csvPath.string()
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
                                   const filesystem::path &csvPath,
                                   const eval::StreamingPointStats &lastPoint) {
            cout << algorithmLogPrefix(spec) << "[stream] csv=" << csvPath.string()
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
                               const filesystem::path &csvPath,
                               const eval::MergePairStats &stats) {
            cout << algorithmLogPrefix(spec) << "[merge] csv=" << csvPath.string()
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
            const filesystem::path csvPath = path_utils::buildResultCsvPath(
                ctx.repoRoot,
                ctx.resultsNamespace,
                spec.algorithmId,
                spec.params,
                spec.hashName,
                mode);
            filesystem::create_directories(csvPath.parent_path());

            if (isStreaming(mode)) {
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

            if (isMerge(mode)) {
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
        void runIfSelected(eval::EvaluationFramework &bench,
                           const DatasetRuntimeContext &ctx,
                           const unordered_set<string> &selected,
                           const string &algorithmId,
                           const string &params,
                           const string &hashName,
                           const double rseTheoretical,
                           const RunMode mode,
                           CtorArgs &&... ctorArgs) {
            if (!shouldRun(selected, algorithmId)) {
                return;
            }

            const AlgorithmRunSpec spec{
                algorithmId,
                params,
                hashName,
                rseTheoretical
            };
            runSingleAlgorithm<Algo>(
                bench,
                ctx,
                spec,
                mode,
                forward<CtorArgs>(ctorArgs)...);
        }
    } // namespace

    void AlgorithmExecutor::run(const RunConfig &cfg,
                                const vector<string> &algs,
                                const RunMode mode) const {
        auto ctx = config::loadDatasetRuntimeContext(cfg);
        auto runtimeHash = satp::hashing::getHashFunctionBy(cfg.hashFunctionName, ctx.seed);
        const string hashName = cfg.hashFunctionName;
        eval::EvaluationFramework bench(move(ctx.index), move(runtimeHash));
        const auto selected = collectRequestedAlgorithms(algs);
        const string kParam = "k=" + to_string(cfg.k);
        const string kAndLLogParam = kParam + ",L=" + to_string(cfg.lLog);
        const string lParam = "L=" + to_string(cfg.l);

        printRunContext(ctx, mode, hashName);

        runIfSelected<alg::HyperLogLogPlusPlus>(
            bench, ctx, selected,
            "hllpp",
            kParam, hashName, rseHll(cfg.k),
            mode, cfg.k);

        runIfSelected<alg::HyperLogLog>(
            bench, ctx, selected,
            "hll",
            kAndLLogParam, hashName, rseHll(cfg.k),
            mode, cfg.k, cfg.lLog);

        runIfSelected<alg::LogLog>(
            bench, ctx, selected,
            "ll",
            kAndLLogParam, hashName, rseLogLog(cfg.k),
            mode, cfg.k, cfg.lLog);

        runIfSelected<alg::ProbabilisticCounting>(
            bench, ctx, selected,
            "pc",
            lParam, hashName, rseUnknown(),
            mode, cfg.l);
    }
} // namespace satp::cli
