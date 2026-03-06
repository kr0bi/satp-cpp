#pragma once

#include <cstddef>
#include <cstdint>

using namespace std;

namespace satp::evaluation {
    struct EvaluationMetadata {
        size_t runs = 0;
        size_t sampleSize = 0;
        size_t distinctCount = 0;
        uint32_t seed = 0;
    };

    [[nodiscard]] inline bool hasEmptyDataset(const EvaluationMetadata &metadata) noexcept {
        return metadata.runs == 0u || metadata.sampleSize == 0u;
    }
} // namespace satp::evaluation
