#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

using namespace std;

namespace satp::evaluation {
    class CheckpointPlanner {
    public:
        [[nodiscard]] static vector<size_t> build(size_t sampleSize, size_t maxCheckpoints) {
            if (sampleSize == 0 || maxCheckpoints == 0) return {};
            if (maxCheckpoints == 1) return {sampleSize};
            if (sampleSize <= maxCheckpoints) {
                vector<size_t> all(sampleSize);
                for (size_t i = 0; i < sampleSize; ++i) all[i] = i + 1u;
                return all;
            }
            const size_t budget = min(sampleSize, maxCheckpoints);
            const size_t warmupPoints = max<size_t>(2u, budget / 3u);
            const size_t warmupEnd = min(sampleSize, max<size_t>(
                warmupPoints,
                static_cast<size_t>(ceil(static_cast<double>(sampleSize) * 1e-2))));
            vector<size_t> candidates;
            candidates.reserve((budget * 4u) / 3u + 4u);
            appendLinear(candidates, 1u, warmupEnd, warmupPoints);
            appendLog(candidates, warmupEnd, sampleSize, budget);
            candidates.push_back(1u);
            candidates.push_back(sampleSize);
            sort(candidates.begin(), candidates.end());
            candidates.erase(unique(candidates.begin(), candidates.end()), candidates.end());
            vector<size_t> checkpoints = compressToBudget(candidates, budget);
            while (checkpoints.size() < budget) {
                const size_t sizeBefore = checkpoints.size();
                densifyLargestGap(checkpoints);
                if (checkpoints.size() == sizeBefore) break;
            }
            return checkpoints;
        }

    private:
        static void appendLinear(vector<size_t> &out, size_t start, size_t end, size_t count) {
            if (count == 0 || start > end) return;
            if (count == 1) return static_cast<void>(out.push_back(end));
            const size_t span = end - start;
            for (size_t i = 0; i < count; ++i) out.push_back(start + ((span * i) / (count - 1u)));
        }

        static void appendLog(vector<size_t> &out, size_t start, size_t end, size_t count) {
            if (count == 0 || start == 0 || start > end) return;
            if (count == 1) return static_cast<void>(out.push_back(end));
            const double ratio = static_cast<double>(end) / static_cast<double>(start);
            for (size_t i = 0; i < count; ++i) {
                const double t = static_cast<double>(i) / static_cast<double>(count - 1u);
                out.push_back(static_cast<size_t>(ceil(static_cast<double>(start) * pow(ratio, t))));
            }
        }

        [[nodiscard]] static vector<size_t> compressToBudget(const vector<size_t> &candidates, size_t budget) {
            if (candidates.size() <= budget) return candidates;
            vector<size_t> out;
            out.reserve(budget);
            out.push_back(candidates.front());
            const size_t lastIndex = candidates.size() - 1u;
            size_t previousIndex = 0u;
            for (size_t i = 1; i + 1u < budget; ++i) {
                size_t index = static_cast<size_t>(llround((static_cast<double>(i) * lastIndex) /
                                                           static_cast<double>(budget - 1u)));
                index = max(index, previousIndex + 1u);
                index = min(index, lastIndex - (budget - 1u - i));
                out.push_back(candidates[index]);
                previousIndex = index;
            }
            out.push_back(candidates.back());
            return out;
        }

        static void densifyLargestGap(vector<size_t> &checkpoints) {
            size_t bestIndex = 0u;
            size_t bestGap = 0u;
            for (size_t i = 1; i < checkpoints.size(); ++i) {
                const size_t gap = checkpoints[i] - checkpoints[i - 1u];
                if (gap > bestGap) {
                    bestGap = gap;
                    bestIndex = i;
                }
            }
            if (bestGap <= 1u) return;
            checkpoints.insert(checkpoints.begin() + static_cast<ptrdiff_t>(bestIndex),
                               checkpoints[bestIndex - 1u] + (bestGap / 2u));
        }
    };
} // namespace satp::evaluation
