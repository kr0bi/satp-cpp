#pragma once

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>

namespace satp::evaluation {
    class StreamingCheckpointBuilder {
    public:
        [[nodiscard]] static std::vector<std::size_t> build(std::size_t sampleSize,
                                                            std::size_t maxCheckpoints) {
            if (sampleSize == 0 || maxCheckpoints == 0) return {};
            if (maxCheckpoints == 1) return {sampleSize};
            if (sampleSize <= maxCheckpoints) {
                std::vector<std::size_t> all(sampleSize);
                for (std::size_t i = 0; i < sampleSize; ++i) {
                    all[i] = i + 1u;
                }
                return all;
            }

            // Hybrid percentage-based strategy:
            // - Phase 1 (dense):   [0%, 0.1%]
            // - Phase 2 (log):     (0.1%, 10%]
            // - Phase 3 (log):     (10%, 100%]
            constexpr double PHASE1_END_RATIO = 1e-3; // 0.1%
            constexpr double PHASE2_END_RATIO = 1e-1; // 10%

            const std::size_t phase1End = std::max<std::size_t>(
                1u,
                static_cast<std::size_t>(std::ceil(static_cast<double>(sampleSize) * PHASE1_END_RATIO)));
            std::vector<std::size_t> checkpoints;
            checkpoints.reserve(maxCheckpoints);
            checkpoints.push_back(1u);

            const std::size_t remaining = maxCheckpoints - 1u;
            std::size_t phase1Count = (remaining * 50u) / 100u;
            std::size_t phase2Count = (remaining * 30u) / 100u;
            std::size_t phase3Count = remaining - phase1Count - phase2Count;

            phase1Count = std::max<std::size_t>(phase1Count, 1u);
            phase2Count = std::max<std::size_t>(phase2Count, 1u);
            if (phase1Count + phase2Count > remaining) {
                phase2Count = remaining - phase1Count;
            }
            phase3Count = remaining - phase1Count - phase2Count;

            const auto appendLinear = [&](std::size_t start, std::size_t end, std::size_t count) {
                if (count == 0 || start > end) return;
                const std::size_t width = end - start + 1u;
                for (std::size_t i = 1; i <= count; ++i) {
                    const std::size_t offset = (i * width + count - 1u) / count; // ceil(i*width/count)
                    const std::size_t pos = std::min(end, start + offset - 1u);
                    checkpoints.push_back(pos);
                }
            };

            const auto appendLogPercent = [&](double startPercent, double endPercent, std::size_t count) {
                if (count == 0 || startPercent <= 0.0 || endPercent <= 0.0 || startPercent >= endPercent) return;
                const double base = endPercent / startPercent;
                for (std::size_t i = 1; i <= count; ++i) {
                    const double t = static_cast<double>(i) / static_cast<double>(count);
                    const double p = startPercent * std::pow(base, t);
                    const auto pos = static_cast<std::size_t>(
                        std::ceil(p * static_cast<double>(sampleSize)));
                    checkpoints.push_back(std::clamp<std::size_t>(pos, 1u, sampleSize));
                }
            };

            appendLinear(1u, phase1End, phase1Count);
            appendLogPercent(PHASE1_END_RATIO, PHASE2_END_RATIO, phase2Count);
            appendLogPercent(PHASE2_END_RATIO, 1.0, phase3Count);

            std::sort(checkpoints.begin(), checkpoints.end());
            checkpoints.erase(std::unique(checkpoints.begin(), checkpoints.end()), checkpoints.end());

            if (checkpoints.back() != sampleSize) {
                checkpoints.push_back(sampleSize);
            }
            return checkpoints;
        }
    };
} // namespace satp::evaluation
