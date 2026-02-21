#pragma once

#include <cmath>
#include <cstddef>

#include "satp/simulation/Stats.h"

namespace satp::evaluation {
    class ErrorAccumulator {
    public:
        void add(double estimate, double truth) {
            ++count_;

            const double delta = estimate - estimateMean_;
            estimateMean_ += delta / static_cast<double>(count_);
            const double delta2 = estimate - estimateMean_;
            estimateM2_ += delta * delta2;

            truthSum_ += truth;

            const double err = estimate - truth;
            absErrSum_ += std::abs(err);
            sqErrSum_ += err * err;
            if (truth > 0.0) {
                absRelErrSum_ += std::abs(err) / truth;
            }
        }

        [[nodiscard]] Stats toStats() const {
            if (count_ == 0) return {};

            const double runs = static_cast<double>(count_);
            const double truthMean = truthSum_ / runs;
            const double variance = (count_ > 1) ? (estimateM2_ / static_cast<double>(count_ - 1)) : 0.0;
            const double stddev = std::sqrt(variance);
            const double bias = estimateMean_ - truthMean;
            const double absoluteBias = std::abs(bias);
            const double relativeBias = (truthMean != 0.0) ? (bias / truthMean) : 0.0;
            const double meanRelativeError = absRelErrSum_ / runs;
            const double rmse = std::sqrt(sqErrSum_ / runs);
            const double mae = absErrSum_ / runs;
            const double rseObserved = (truthMean != 0.0) ? (stddev / truthMean) : 0.0;

            Stats stats{};
            stats.mean = estimateMean_;
            stats.variance = variance;
            stats.bias = bias;
            stats.absolute_bias = absoluteBias;
            stats.mean_relative_error = meanRelativeError;
            stats.relative_bias = relativeBias;
            stats.rmse = rmse;
            stats.mae = mae;
            stats.stddev = stddev;
            stats.rse_observed = rseObserved;
            stats.truth_mean = truthMean;
            return stats;
        }

        [[nodiscard]] StreamingPointStats toStreamingPoint(const std::size_t elementIndex) const {
            const Stats stats = toStats();
            StreamingPointStats point{};
            point.number_of_elements_processed = elementIndex;
            point.mean = stats.mean;
            point.variance = stats.variance;
            point.bias = stats.bias;
            point.absolute_bias = stats.absolute_bias;
            point.mean_relative_error = stats.mean_relative_error;
            point.relative_bias = stats.relative_bias;
            point.rmse = stats.rmse;
            point.mae = stats.mae;
            point.stddev = stats.stddev;
            point.rse_observed = stats.rse_observed;
            point.truth_mean = stats.truth_mean;
            return point;
        }

    private:
        std::size_t count_ = 0;
        double estimateMean_ = 0.0;
        double estimateM2_ = 0.0;
        double truthSum_ = 0.0;
        double absErrSum_ = 0.0;
        double sqErrSum_ = 0.0;
        double absRelErrSum_ = 0.0;
    };
} // namespace satp::evaluation
