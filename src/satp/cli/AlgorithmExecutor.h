#pragma once

#include <string>
#include <vector>

#include "satp/cli/CliTypes.h"

namespace satp::cli {
    class AlgorithmExecutor {
    public:
        void run(const RunConfig &cfg,
                 const std::vector<std::string> &algs,
                 RunMode mode) const;
    };
} // namespace satp::cli
