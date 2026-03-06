#pragma once

#include <string>
#include <vector>

#include "satp/cli/detail/CliTypes.h"
#include "satp/cli/detail/execution/AlgorithmRunner.h"
#include "satp/simulation/Simulation.h"

using namespace std;

namespace satp::cli::executor {
    [[nodiscard]] vector<AlgorithmJob> buildAlgorithmJobs(
        satp::evaluation::EvaluationFramework &bench,
        const DatasetRuntimeContext &ctx,
        const RunConfig &cfg,
        RunMode mode,
        const string &hashName);
} // namespace satp::cli::executor
