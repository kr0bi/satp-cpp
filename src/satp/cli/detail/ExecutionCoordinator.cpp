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
        const string hashName = cfg.hashFunctionName;
        satp::evaluation::EvaluationFramework bench(std::move(ctx.index), std::move(runtimeHash));

        executor::printRunContext(ctx, mode, hashName);
        const auto selected = executor::collectRequestedAlgorithms(algs);
        const auto jobs = executor::buildAlgorithmJobs(bench, ctx, cfg, mode, hashName);

        for (const auto &job : jobs) {
            if (!executor::shouldRun(selected, job.spec.algorithmId)) {
                continue;
            }
            job.run(job.spec);
        }
    }
} // namespace satp::cli
