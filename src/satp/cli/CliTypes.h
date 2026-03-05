#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "satp/hashing/HashFactory.h"
#include "satp/hashing/HashFunction.h"
#include "satp/io/BinaryDatasetIO.h"

namespace satp::cli {
    struct RunConfig {
        std::string datasetPath = "dataset.bin";
        std::string resultsNamespace = "legacy";
        std::reference_wrapper<const hashing::HashFunction> hashFunction = std::cref(hashing::defaultHashFunction());
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
        Streaming,
        Merge
    };

    struct DatasetView {
        std::size_t sampleSize = 0;
        std::size_t runs = 0;
        std::uint32_t seed = 0;
    };

    struct DatasetRuntimeContext {
        io::BinaryDatasetIndex index;
        std::size_t sampleSize = 0;
        std::size_t runs = 0;
        std::uint32_t seed = 0;
        std::string resultsNamespace;
        std::filesystem::path repoRoot;
    };

    struct AlgorithmRunSpec {
        std::string key;
        std::string displayTag;
        std::string algorithmName;
        std::string params;
        std::string hashName;
        double rseTheoretical = 0.0;
    };
} // namespace satp::cli
