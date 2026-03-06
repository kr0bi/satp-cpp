#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "satp/ProgressBar.h"
#include "satp/simulation/Stats.h"

using namespace std;

namespace satp::evaluation::detail {
    template<typename Algo>
    concept MergeableAlgorithm = requires(Algo a, const Algo &b) {
        { a.merge(b) } -> same_as<void>;
    };

    [[nodiscard]] inline satp::util::ProgressBar makeProgressBar(const size_t totalTicks) {
        return satp::util::ProgressBar(totalTicks, cout, 50, 10'000);
    }

    inline void finishProgressBar(satp::util::ProgressBar &bar) {
        bar.finish();
        cout.flush();
    }

    template<typename Algo>
    inline void processValues(Algo &algo,
                              const vector<uint32_t> &values,
                              satp::util::ProgressBar &bar) {
        for (const auto value : values) {
            algo.process(value);
            bar.tick();
        }
    }

    inline void validateStreamingPartition(const vector<uint32_t> &values,
                                           const vector<uint8_t> &truthBits,
                                           const size_t sampleSize) {
        if (values.size() != sampleSize) {
            throw runtime_error("Invalid binary dataset: partition size mismatch while streaming");
        }
        if (truthBits.size() != (sampleSize + 7u) / 8u) {
            throw runtime_error("Invalid binary dataset: truth bitset size mismatch while streaming");
        }
    }

    [[nodiscard]] inline bool truthBitIsSet(const vector<uint8_t> &truthBits,
                                            const size_t index) noexcept {
        const uint8_t byte = truthBits[index >> 3u];
        return ((byte >> (index & 7u)) & 0x1u) != 0;
    }

    inline MergePairStats summarizeMergePairs(const vector<MergePairPoint> &points) {
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
} // namespace satp::evaluation::detail
