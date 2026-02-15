#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace satp::evaluation {
    class StreamingCheckpointBuilder {
    public:
        [[nodiscard]] static std::vector<std::size_t> build(std::size_t sampleSize,
                                                            std::size_t maxCheckpoints) {
            if (sampleSize == 0 || maxCheckpoints == 0) return {};

            const std::size_t checkpointCount = std::min(sampleSize, maxCheckpoints);
            std::vector<std::size_t> checkpoints;
            checkpoints.reserve(checkpointCount);
            for (std::size_t i = 1; i <= checkpointCount; ++i) {
                // ceil(i * n / checkpointCount), 1-based index
                const std::size_t pos = (i * sampleSize + checkpointCount - 1) / checkpointCount;
                checkpoints.push_back(pos);
            }
            return checkpoints;
        }
    };
} // namespace satp::evaluation
