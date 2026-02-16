#include "satp/cli/PathUtils.h"

#include <stdexcept>

namespace satp::cli::path_utils {
    std::string sanitizeForPath(std::string value) {
        for (char &c : value) {
            const bool keep = (c >= 'a' && c <= 'z') ||
                              (c >= 'A' && c <= 'Z') ||
                              (c >= '0' && c <= '9');
            if (!keep) c = '_';
        }

        std::string out;
        out.reserve(value.size());
        bool prevUnderscore = false;
        for (const char c : value) {
            if (c == '_') {
                if (!prevUnderscore) out.push_back(c);
                prevUnderscore = true;
            } else {
                out.push_back(c);
                prevUnderscore = false;
            }
        }

        while (!out.empty() && out.front() == '_') out.erase(out.begin());
        while (!out.empty() && out.back() == '_') out.pop_back();
        return out.empty() ? "default" : out;
    }

    std::optional<std::filesystem::path> tryFindRepoRoot(const std::filesystem::path &start) {
        namespace fs = std::filesystem;
        if (start.empty()) return std::nullopt;

        fs::path path = fs::absolute(start);
        if (fs::is_regular_file(path)) path = path.parent_path();

        while (!path.empty()) {
            if (fs::exists(path / "CMakeLists.txt") && fs::exists(path / "src")) {
                return path;
            }
            if (path == path.root_path()) break;
            path = path.parent_path();
        }

        return std::nullopt;
    }

    std::filesystem::path detectRepoRoot(const std::filesystem::path &datasetPath) {
        if (auto fromDataset = tryFindRepoRoot(datasetPath)) {
            return *fromDataset;
        }
        if (auto fromCwd = tryFindRepoRoot(std::filesystem::current_path())) {
            return *fromCwd;
        }
        throw std::runtime_error("Impossibile individuare la root della repository");
    }

    std::filesystem::path buildResultCsvPath(const std::filesystem::path &repoRoot,
                                             const std::string &algorithmName,
                                             const std::string &params,
                                             const RunMode mode) {
        const std::string paramsDir = sanitizeForPath(params);
        std::string fileName = "results_oneshot.csv";
        if (mode == RunMode::Streaming) {
            fileName = "results_streaming.csv";
        } else if (mode == RunMode::Merge) {
            fileName = "results_merge.csv";
        }
        return repoRoot / "results" / algorithmName / paramsDir / fileName;
    }
} // namespace satp::cli::path_utils
