#pragma once

#include <optional>
#include <string>

#include "satp/cli/CliTypes.h"

namespace satp::cli::config {
    [[nodiscard]] Command parseCommand(const std::string &line);

    [[nodiscard]] bool setParam(RunConfig &cfg,
                                const std::string &param,
                                const std::string &value);

    void printHelp();
    void printAlgorithms();
    void printConfig(const RunConfig &cfg);

    [[nodiscard]] std::optional<DatasetView> readDatasetView(
        const std::string &datasetPath);

    [[nodiscard]] DatasetRuntimeContext loadDatasetRuntimeContext(
        const RunConfig &cfg);
} // namespace satp::cli::config
