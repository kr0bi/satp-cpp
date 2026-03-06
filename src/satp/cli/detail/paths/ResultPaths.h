#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "satp/cli/detail/CliTypes.h"

using namespace std;

namespace satp::cli::path_utils {
    [[nodiscard]] string sanitizeForPath(string value);

    [[nodiscard]] optional<filesystem::path> tryFindRepoRoot(
        const filesystem::path &start);

    [[nodiscard]] filesystem::path detectRepoRoot(
        const filesystem::path &datasetPath);

    [[nodiscard]] filesystem::path buildResultCsvPath(
        const filesystem::path &repoRoot,
        const string &resultsNamespace,
        const string &algorithmId,
        const string &params,
        const string &hashName,
        RunMode mode);
} // namespace satp::cli::path_utils
