#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "satp/dataset/Dataset.h"

using namespace std;

namespace satp::cli {
    struct RunConfig {
        string datasetPath = "dataset.bin";
        string resultsNamespace = "legacy";
        string hashFunctionName = "splitmix64";
        uint32_t k = 16;    // registers for HLL/LogLog
        uint32_t l = 16;    // bitmap size for PC
        uint32_t lLog = 32; // bitmap size for LogLog internals
    };

    struct Command {
        string name;
        vector<string> args;
    };

    enum class RunMode {
        Streaming,
        Merge
    };

    struct DatasetView {
        size_t sampleSize = 0;
        size_t runs = 0;
        uint32_t seed = 0;
    };

    struct DatasetRuntimeContext {
        dataset::DatasetIndex index;
        size_t sampleSize = 0;
        size_t runs = 0;
        uint32_t seed = 0;
        string resultsNamespace;
        filesystem::path repoRoot;
    };

    struct AlgorithmRunSpec {
        string algorithmId;
        string params;
        string hashName;
        double rseTheoretical = 0.0;
    };
} // namespace satp::cli
