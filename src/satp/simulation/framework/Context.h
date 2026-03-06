#pragma once

#include <cstdint>

#include "satp/hashing/HashFunction.h"
#include "satp/io/BinaryDatasetIO.h"
#include "satp/simulation/framework/EvaluationMetadata.h"
#include "satp/simulation/framework/ProgressCallbacks.h"

using namespace std;

namespace satp::evaluation::detail {
    struct EvaluationContext {
        const io::BinaryDatasetIndex &binaryDataset;
        const EvaluationMetadata &metadata;
        const hashing::HashFunction &hashFunction;
        const ProgressCallbacks *progress = nullptr;
    };
} // namespace satp::evaluation::detail
