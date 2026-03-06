#pragma once

// This module exposes the interactive benchmark CLI used to configure
// experiments and launch streaming or merge evaluations on sketching algorithms.

#include "satp/cli/detail/CliTypes.h"
#include "satp/cli/detail/ExecutionCoordinator.h"

namespace satp::cli {
    class Cli {
    public:
        int run();

    private:
        RunConfig config_;
        ExecutionCoordinator executor_;
    };
} // namespace satp::cli
