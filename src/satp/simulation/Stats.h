#pragma once

#include <cstddef>

namespace satp::evaluation {
    struct Stats {
        double difference = 0.0;
        double mean = 0.0;
        double variance = 0.0;
        double bias = 0.0;
        double mean_relative_error = 0.0;
        double bias_relative = 0.0;
        double rmse = 0.0;
        double mae = 0.0;
        double stddev = 0.0;
        double rse_observed = 0.0;
        double truth_mean = 0.0; // \bar{F}_0
    };

    struct StreamingPointStats {
        std::size_t element_index = 0; // 1-based index t
        double difference = 0.0;
        double mean = 0.0; // \bar{\hat{F}_0(t)}
        double variance = 0.0;
        double bias = 0.0;
        double mean_relative_error = 0.0;
        double bias_relative = 0.0;
        double rmse = 0.0;
        double mae = 0.0;
        double stddev = 0.0;
        double rse_observed = 0.0;
        double truth_mean = 0.0; // \bar{F_0(t)}
    };

    struct MergePairPoint {
        std::size_t pair_index = 0;
        double estimate_merge = 0.0;
        double estimate_serial = 0.0;
        double delta_merge_serial_abs = 0.0;
        double delta_merge_serial_rel = 0.0;
    };

    struct MergePairStats {
        std::size_t pair_count = 0;
        double estimate_merge_mean = 0.0;
        double estimate_serial_mean = 0.0;
        double delta_merge_serial_abs_mean = 0.0;
        double delta_merge_serial_abs_max = 0.0;
        double delta_merge_serial_rel_mean = 0.0;
        double delta_merge_serial_rmse = 0.0;
    };
} // namespace satp::evaluation
