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
            const double difference = std::abs(bias);
            const double biasRelative = (truthMean != 0.0) ? (bias / truthMean) : 0.0;
            const double meanRelativeError = absRelErrSum_ / runs;
            const double rmse = std::sqrt(sqErrSum_ / runs);
            const double mae = absErrSum_ / runs;
            const double rseObserved = (truthMean != 0.0) ? (stddev / truthMean) : 0.0;

            return {
                difference,
                estimateMean_,
                variance,
                bias,
                meanRelativeError,
                biasRelative,
                rmse,
                mae,
                stddev,
                rseObserved,
                truthMean
            };
        }

        [[nodiscard]] StreamingPointStats toStreamingPoint(const std::size_t elementIndex) const {
            const Stats stats = toStats();
            return {
                elementIndex,
                stats.difference,
                stats.mean,
                stats.variance,
                stats.bias,
                stats.mean_relative_error,
                stats.bias_relative,
                stats.rmse,
                stats.mae,
                stats.stddev,
                stats.rse_observed,
                stats.truth_mean
            };
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
