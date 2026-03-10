#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "satp/dataset/Dataset.h"
#include "satp/simulation/detail/merge/HeterogeneousMergeTypes.h"

using namespace std;

namespace satp::cli {
    struct RunConfig {
        string datasetPath = "dataset.bin";
        string resultsNamespace = "legacy";
        string hashFunctionName = "splitmix64";
        optional<string> leftHashFunctionName = nullopt;
        optional<string> rightHashFunctionName = nullopt;
        optional<uint32_t> leftHashSeed = nullopt;
        optional<uint32_t> rightHashSeed = nullopt;
        uint32_t k = 16;    // registers for HLL/LogLog
        optional<uint32_t> leftK = nullopt;
        optional<uint32_t> rightK = nullopt;
        uint32_t l = 16;    // bitmap size for PC
        uint32_t lLog = 32; // bitmap size for LogLog internals
        satp::evaluation::MergeStrategy mergeStrategy = satp::evaluation::MergeStrategy::Direct;
    };

    struct Command {
        string name;
        vector<string> args;
    };

    enum class RunMode {
        Streaming,
        Merge,
        MergeHeterogeneous
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
