#include "satp/cli/detail/execution/RunReporter.h"

#include <cmath>
#include <iostream>
#include <limits>

#include "satp/algorithms/AlgorithmCatalog.h"

using namespace std;

namespace satp::cli::executor {
    const char *modeLabel(const RunMode mode) {
        if (mode == RunMode::Streaming) return "streaming";
        return "merge";
    }

    double rseHll(const uint32_t k) {
        const double m = static_cast<double>(1u << k);
        return 1.04 / sqrt(m);
    }

    double rseLogLog(const uint32_t k) {
        const double m = static_cast<double>(1u << k);
        return 1.30 / sqrt(m);
    }

    double rseUnknown() {
        return numeric_limits<double>::quiet_NaN();
    }

    string algorithmLogPrefix(const AlgorithmRunSpec &spec) {
        return "[" + spec.algorithmId + "|" + satp::algorithms::catalog::getNameBy(spec.algorithmId) + "]";
    }

    void printRunContext(const DatasetRuntimeContext &ctx,
                         const RunMode mode,
                         const string &hashName) {
        cout << "mode: " << modeLabel(mode)
                  << '\t' << "sampleSize: " << ctx.sampleSize
                  << '\t' << "runs: " << ctx.runs
                  << '\t' << "seed: " << ctx.seed
                  << '\t' << "hash: " << hashName << '\n'
                  << "resultsRoot: " << (ctx.repoRoot / "results" / ctx.resultsNamespace).string() << '\n';
    }

    void printStreamingSummary(const AlgorithmRunSpec &spec,
                               const filesystem::path &csvPath,
                               const satp::evaluation::StreamingPointStats &lastPoint) {
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
                           const satp::evaluation::MergePairStats &stats) {
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
} // namespace satp::cli::executor
