#pragma once

#include <string>
#include <vector>

#include "satp/cli/CliTypes.h"

using namespace std;

namespace satp::cli {
    class AlgorithmExecutor {
    public:
        void run(const RunConfig &cfg,
                 const vector<string> &algs,
                 RunMode mode) const;
    };
} // namespace satp::cli
