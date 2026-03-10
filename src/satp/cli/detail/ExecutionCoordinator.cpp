#include "satp/cli/detail/ExecutionCoordinator.h"

#include <utility>

#include "satp/cli/detail/config/DatasetRuntime.h"
#include "satp/cli/detail/execution/AlgorithmSelection.h"
#include "satp/cli/detail/execution/JobFactory.h"
#include "satp/cli/detail/execution/RunReporter.h"
#include "satp/hashing/HashFactory.h"
#include "satp/simulation/Simulation.h"

using namespace std;

namespace satp::cli {
    void ExecutionCoordinator::run(const RunConfig &cfg,
                                   const vector<string> &algs,
                                   const RunMode mode) const {
        auto ctx = config::loadDatasetRuntimeContext(cfg);
        auto runtimeHash = satp::hashing::getHashFunctionBy(cfg.hashFunctionName, ctx.seed);
        satp::evaluation::EvaluationFramework bench(std::move(ctx.index), std::move(runtimeHash));

        const auto selected = executor::collectRequestedAlgorithms(algs);
        vector<executor::AlgorithmJob> jobs;
        string hashLabel = cfg.hashFunctionName;
        if (mode == RunMode::MergeHeterogeneous) {
            const string leftHash = cfg.leftHashFunctionName.value_or(cfg.hashFunctionName);
            const string rightHash = cfg.rightHashFunctionName.value_or(cfg.hashFunctionName);
            hashLabel = leftHash + "->" + rightHash;
            jobs = executor::buildHeterogeneousMergeJobs(bench, ctx, cfg);
        } else {
            jobs = executor::buildAlgorithmJobs(bench, ctx, cfg, mode, cfg.hashFunctionName);
        }

        executor::printRunContext(ctx, mode, hashLabel);

        for (const auto &job : jobs) {
            if (!executor::shouldRun(selected, job.spec.algorithmId)) {
                continue;
            }
            job.run(job.spec);
        }
    }
} // namespace satp::cli
