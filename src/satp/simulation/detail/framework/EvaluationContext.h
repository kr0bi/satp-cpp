#pragma once

#include <cstdint>

#include "satp/hashing/HashFunction.h"
#include "satp/dataset/Dataset.h"
#include "satp/simulation/detail/framework/EvaluationMetadata.h"
#include "satp/simulation/detail/framework/ProgressCallbacks.h"

using namespace std;

namespace satp::evaluation::detail {
    struct EvaluationContext {
        const dataset::DatasetIndex &binaryDataset;
        const EvaluationMetadata &metadata;
        const hashing::HashFunction &hashFunction;
        const ProgressCallbacks *progress = nullptr;
    };
} // namespace satp::evaluation::detail
