#pragma once

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <vector>

using namespace std;

namespace satp::evaluation {
    class StreamingCheckpointBuilder {
    public:
        [[nodiscard]] static vector<size_t> build(size_t sampleSize,
                                                            size_t maxCheckpoints) {
            if (sampleSize == 0 || maxCheckpoints == 0) return {};
            if (maxCheckpoints == 1) return {sampleSize};
            if (sampleSize <= maxCheckpoints) {
                vector<size_t> all(sampleSize);
                for (size_t i = 0; i < sampleSize; ++i) {
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

            const size_t phase1End = max<size_t>(
                1u,
                static_cast<size_t>(ceil(static_cast<double>(sampleSize) * PHASE1_END_RATIO)));
            vector<size_t> checkpoints;
            checkpoints.reserve(maxCheckpoints);
            checkpoints.push_back(1u);

            const size_t remaining = maxCheckpoints - 1u;
            size_t phase1Count = (remaining * 50u) / 100u;
            size_t phase2Count = (remaining * 30u) / 100u;
            size_t phase3Count = remaining - phase1Count - phase2Count;

            phase1Count = max<size_t>(phase1Count, 1u);
            phase2Count = max<size_t>(phase2Count, 1u);
            if (phase1Count + phase2Count > remaining) {
                phase2Count = remaining - phase1Count;
            }
            phase3Count = remaining - phase1Count - phase2Count;

            const auto appendLinear = [&](size_t start, size_t end, size_t count) {
                if (count == 0 || start > end) return;
                const size_t width = end - start + 1u;
                for (size_t i = 1; i <= count; ++i) {
                    const size_t offset = (i * width + count - 1u) / count; // ceil(i*width/count)
                    const size_t pos = min(end, start + offset - 1u);
                    checkpoints.push_back(pos);
                }
            };

            const auto appendLogPercent = [&](double startPercent, double endPercent, size_t count) {
                if (count == 0 || startPercent <= 0.0 || endPercent <= 0.0 || startPercent >= endPercent) return;
                const double base = endPercent / startPercent;
                for (size_t i = 1; i <= count; ++i) {
                    const double t = static_cast<double>(i) / static_cast<double>(count);
                    const double p = startPercent * pow(base, t);
                    const auto pos = static_cast<size_t>(
                        ceil(p * static_cast<double>(sampleSize)));
                    checkpoints.push_back(clamp<size_t>(pos, 1u, sampleSize));
                }
            };

            appendLinear(1u, phase1End, phase1Count);
            appendLogPercent(PHASE1_END_RATIO, PHASE2_END_RATIO, phase2Count);
            appendLogPercent(PHASE2_END_RATIO, 1.0, phase3Count);

            ranges::sort(checkpoints);
            checkpoints.erase(ranges::unique(checkpoints).begin(), checkpoints.end());

            if (checkpoints.back() != sampleSize) {
                checkpoints.push_back(sampleSize);
            }
            return checkpoints;
        }
    };
} // namespace satp::evaluation
