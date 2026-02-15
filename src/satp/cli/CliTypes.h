#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "satp/io/BinaryDatasetIO.h"

namespace satp::cli {
    struct RunConfig {
        std::string datasetPath = "dataset.bin";
        std::uint32_t k = 16;    // registers for HLL/LogLog
        std::uint32_t l = 16;    // bitmap size for PC
        std::uint32_t lLog = 32; // bitmap size for LogLog internals
    };

    struct Command {
        std::string name;
        std::vector<std::string> args;
    };

    enum class RunMode {
        Normal,
        Streaming
    };

    struct DatasetView {
        std::size_t sampleSize = 0;
        std::size_t runs = 0;
        std::uint32_t seed = 0;
    };

    struct DatasetRuntimeContext {
        satp::io::BinaryDatasetIndex index;
        std::size_t sampleSize = 0;
        std::size_t runs = 0;
        std::uint32_t seed = 0;
        std::filesystem::path repoRoot;
    };

    struct AlgorithmRunSpec {
        std::string key;
        std::string displayTag;
        std::string algorithmName;
        std::string params;
        double rseTheoretical = 0.0;
    };
} // namespace satp::cli
