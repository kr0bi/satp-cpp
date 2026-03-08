#pragma once

#include <utility>

using namespace std;

namespace satp::evaluation {
    template<typename Algo, typename... Args>
    vector<StreamingPointStats> EvaluationFramework::evaluateStreaming(Args &&... ctorArgs) const {
        const auto evaluationContext = context();
        return modes::streaming::evaluate<Algo>(evaluationContext, std::forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    vector<StreamingPointStats> EvaluationFramework::evaluateStreaming(const ProgressCallbacks &progress,
                                                                       Args &&... ctorArgs) const {
        const auto evaluationContext = context(&progress);
        return modes::streaming::evaluate<Algo>(evaluationContext, std::forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    vector<MergePairPoint> EvaluationFramework::evaluateMergePairs(Args &&... ctorArgs) const {
        const auto evaluationContext = context();
        return modes::merge::evaluate<Algo>(evaluationContext, std::forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    vector<MergePairPoint> EvaluationFramework::evaluateMergePairs(const ProgressCallbacks &progress,
                                                                   Args &&... ctorArgs) const {
        const auto evaluationContext = context(&progress);
        return modes::merge::evaluate<Algo>(evaluationContext, std::forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename Builder>
    vector<HeterogeneousMergePoint> EvaluationFramework::evaluateHeterogeneousMergePairs(
        const HeterogeneousMergeRunDescriptor &descriptor,
        Builder buildAlgo) const {
        const auto evaluationContext = context();
        return modes::merge_heterogeneous::evaluate<Algo>(evaluationContext, descriptor, buildAlgo);
    }

    template<typename Algo, typename Builder>
    vector<HeterogeneousMergePoint> EvaluationFramework::evaluateHeterogeneousMergePairs(
        const HeterogeneousMergeRunDescriptor &descriptor,
        const ProgressCallbacks &progress,
        Builder buildAlgo) const {
        const auto evaluationContext = context(&progress);
        return modes::merge_heterogeneous::evaluate<Algo>(evaluationContext, descriptor, buildAlgo);
    }
} // namespace satp::evaluation
