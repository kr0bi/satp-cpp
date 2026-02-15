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
} // namespace satp::evaluation
