#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "satp/simulation/detail/framework/EvaluationMetadata.h"

using namespace std;

namespace satp::evaluation {
    enum class MergeValidity {
        Valid,
        Recoverable,
        Invalid
    };

    enum class MergeStrategy {
        Direct,
        Reject,
        ReduceThenMerge,
        UnsafeNaiveMerge
    };

    enum class MergeTopology {
        Pairwise,
        BalancedTree,
        LeftDeepChain,
        Custom
    };

    [[nodiscard]] inline string_view toString(const MergeValidity validity) noexcept {
        switch (validity) {
            case MergeValidity::Valid: return "valid";
            case MergeValidity::Recoverable: return "recoverable";
            case MergeValidity::Invalid: return "invalid";
        }
        return "invalid";
    }

    [[nodiscard]] inline string_view toString(const MergeStrategy strategy) noexcept {
        switch (strategy) {
            case MergeStrategy::Direct: return "direct";
            case MergeStrategy::Reject: return "reject";
            case MergeStrategy::ReduceThenMerge: return "reduce_then_merge";
            case MergeStrategy::UnsafeNaiveMerge: return "unsafe_naive_merge";
        }
        return "reject";
    }

    [[nodiscard]] inline string_view toString(const MergeTopology topology) noexcept {
        switch (topology) {
            case MergeTopology::Pairwise: return "pairwise";
            case MergeTopology::BalancedTree: return "balanced_tree";
            case MergeTopology::LeftDeepChain: return "left_deep_chain";
            case MergeTopology::Custom: return "custom";
        }
        return "custom";
    }

    struct MergeSketchContext {
        string hashName;
        uint64_t hashSeed = 0;
        string params;
    };

    struct HeterogeneousMergeRunDescriptor {
        string algorithmName;
        MergeSketchContext left;
        MergeSketchContext right;
        MergeStrategy strategy = MergeStrategy::Reject;
        MergeValidity validity = MergeValidity::Invalid;
        MergeTopology topology = MergeTopology::Pairwise;
        EvaluationMetadata metadata;
    };

    struct HeterogeneousMergePoint {
        size_t pair_index = 0;
        double exact_union = 0.0;
        double estimate_merge = 0.0;
        double estimate_serial = 0.0;
        double error_merge_abs_exact = 0.0;
        double error_merge_rel_exact = 0.0;
        double error_serial_abs_exact = 0.0;
        double error_serial_rel_exact = 0.0;
        double baseline_homogeneous = numeric_limits<double>::quiet_NaN();
        double delta_vs_baseline = numeric_limits<double>::quiet_NaN();
    };
} // namespace satp::evaluation
