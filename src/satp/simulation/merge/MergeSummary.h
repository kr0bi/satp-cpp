#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "satp/simulation/metrics/Stats.h"

using namespace std;

namespace satp::evaluation {
    [[nodiscard]] inline MergePairStats summarizeMergePairs(const vector<MergePairPoint> &points) {
        if (points.empty()) return {};

        MergePairStats out{};
        out.pair_count = points.size();

        double mergeSum = 0.0;
        double serialSum = 0.0;
        double absSum = 0.0;
        double relSum = 0.0;
        double sqAbsSum = 0.0;
        double absMax = 0.0;
        for (const auto &point : points) {
            mergeSum += point.estimate_merge;
            serialSum += point.estimate_serial;
            absSum += point.delta_merge_serial_abs;
            relSum += point.delta_merge_serial_rel;
            sqAbsSum += point.delta_merge_serial_abs * point.delta_merge_serial_abs;
            absMax = max(absMax, point.delta_merge_serial_abs);
        }

        const double n = static_cast<double>(points.size());
        out.estimate_merge_mean = mergeSum / n;
        out.estimate_serial_mean = serialSum / n;
        out.delta_merge_serial_abs_mean = absSum / n;
        out.delta_merge_serial_abs_max = absMax;
        out.delta_merge_serial_rel_mean = relSum / n;
        out.delta_merge_serial_rmse = sqrt(sqAbsSum / n);
        return out;
    }
} // namespace satp::evaluation
