#pragma once

#include <optional>
#include <string>

#include "satp/cli/CliTypes.h"

using namespace std;

namespace satp::cli::config {
    [[nodiscard]] Command parseCommand(const string &line);

    [[nodiscard]] bool setParam(RunConfig &cfg,
                                const string &param,
                                const string &value);

    void printHelp();
    void printAlgorithms();
    void printConfig(const RunConfig &cfg);

    [[nodiscard]] optional<DatasetView> readDatasetView(
        const string &datasetPath);

    [[nodiscard]] DatasetRuntimeContext loadDatasetRuntimeContext(
        const RunConfig &cfg);
} // namespace satp::cli::config
