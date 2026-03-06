#pragma once

#include "satp/cli/CliTypes.h"

namespace satp::cli::config {
    void printHelp();
    void printAlgorithms();
    void printConfig(const RunConfig &cfg);
} // namespace satp::cli::config
