#pragma once

#include <string>
#include <vector>

#include "satp/cli/CliTypes.h"
#include "satp/cli/executor/Runner.h"
#include "satp/simulation/framework/EvaluationFramework.h"

using namespace std;

namespace satp::cli::executor {
    [[nodiscard]] vector<AlgorithmJob> buildAlgorithmJobs(
        satp::evaluation::EvaluationFramework &bench,
        const DatasetRuntimeContext &ctx,
        const RunConfig &cfg,
        RunMode mode,
        const string &hashName);
} // namespace satp::cli::executor
