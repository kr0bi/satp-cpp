#include "satp/cli/AlgorithmExecutor.h"

#include <utility>

#include "satp/cli/config/RuntimeContext.h"
#include "satp/cli/executor/JobFactory.h"
#include "satp/cli/executor/Output.h"
#include "satp/cli/executor/Selection.h"
#include "satp/hashing/HashFactory.h"
#include "satp/simulation/EvaluationFramework.h"

using namespace std;

namespace satp::cli {
    void AlgorithmExecutor::run(const RunConfig &cfg,
                                const vector<string> &algs,
                                const RunMode mode) const {
        auto ctx = config::loadDatasetRuntimeContext(cfg);
        auto runtimeHash = satp::hashing::getHashFunctionBy(cfg.hashFunctionName, ctx.seed);
        const string hashName = cfg.hashFunctionName;
        satp::evaluation::EvaluationFramework bench(move(ctx.index), move(runtimeHash));

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
