#pragma once

#include <optional>
#include <string>

#include "satp/cli/CliTypes.h"

using namespace std;

namespace satp::cli::config {
    [[nodiscard]] optional<DatasetView> readDatasetView(
        const string &datasetPath);

    [[nodiscard]] DatasetRuntimeContext loadDatasetRuntimeContext(
        const RunConfig &cfg);
} // namespace satp::cli::config
