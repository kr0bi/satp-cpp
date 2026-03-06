#include "satp/cli/detail/execution/JobFactory.h"

#include "satp/algorithms/HyperLogLog.h"
#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/algorithms/LogLog.h"
#include "satp/algorithms/ProbabilisticCounting.h"
#include "satp/cli/detail/execution/RunReporter.h"

using namespace std;

namespace satp::cli::executor {
    namespace {
        namespace alg = satp::algorithms;
    } // namespace

    vector<AlgorithmJob> buildAlgorithmJobs(
        satp::evaluation::EvaluationFramework &bench,
        const DatasetRuntimeContext &ctx,
        const RunConfig &cfg,
        const RunMode mode,
        const string &hashName) {
        const string kParam = "k=" + to_string(cfg.k);
        const string kAndLLogParam = kParam + ",L=" + to_string(cfg.lLog);
        const string lParam = "L=" + to_string(cfg.l);

        vector<AlgorithmJob> jobs;
        jobs.reserve(4);

        addAlgorithmJob<alg::HyperLogLogPlusPlus>(
            jobs,
            bench,
            ctx,
            mode,
            "hllpp",
            kParam,
            hashName,
            rseHll(cfg.k),
            cfg.k);

        addAlgorithmJob<alg::HyperLogLog>(
            jobs,
            bench,
            ctx,
            mode,
            "hll",
            kAndLLogParam,
            hashName,
            rseHll(cfg.k),
            cfg.k,
            cfg.lLog);

        addAlgorithmJob<alg::LogLog>(
            jobs,
            bench,
            ctx,
            mode,
            "ll",
            kAndLLogParam,
            hashName,
            rseLogLog(cfg.k),
            cfg.k,
            cfg.lLog);

        addAlgorithmJob<alg::ProbabilisticCounting>(
            jobs,
            bench,
            ctx,
            mode,
            "pc",
            lParam,
            hashName,
            rseUnknown(),
            cfg.l);

        return jobs;
    }
} // namespace satp::cli::executor
