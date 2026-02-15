#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "satp/cli/CliTypes.h"

namespace satp::cli::path_utils {
    [[nodiscard]] std::string sanitizeForPath(std::string value);

    [[nodiscard]] std::optional<std::filesystem::path> tryFindRepoRoot(
        const std::filesystem::path &start);

    [[nodiscard]] std::filesystem::path detectRepoRoot(
        const std::filesystem::path &datasetPath);

    [[nodiscard]] std::filesystem::path buildResultCsvPath(
        const std::filesystem::path &repoRoot,
        const std::string &algorithmName,
        const std::string &params,
        RunMode mode);
} // namespace satp::cli::path_utils
