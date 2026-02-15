#pragma once

#include "satp/cli/AlgorithmExecutor.h"
#include "satp/cli/CliTypes.h"

namespace satp::cli {
    class BenchmarkCli {
    public:
        int run();

    private:
        RunConfig cfg_;
        AlgorithmExecutor executor_;
    };
} // namespace satp::cli
