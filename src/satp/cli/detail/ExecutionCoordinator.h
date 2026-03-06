#pragma once

#include <string>
#include <vector>

#include "satp/cli/detail/CliTypes.h"

using namespace std;

namespace satp::cli {
    class ExecutionCoordinator {
    public:
        void run(const RunConfig &cfg,
                 const vector<string> &algs,
                 RunMode mode) const;
    };
} // namespace satp::cli
