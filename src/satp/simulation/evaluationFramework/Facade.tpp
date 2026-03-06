#pragma once

#include <utility>

using namespace std;

namespace satp::evaluation {
    template<typename Algo, typename... Args>
    Stats EvaluationFramework::evaluate(size_t runs,
                                        size_t sampleSize,
                                        Args &&... ctorArgs) const {
        if (detail::scopeIsEmpty(runs, sampleSize)) return {};
        const auto evaluationContext = context();
        return modes::normal::evaluate<Algo>(evaluationContext, forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    Stats EvaluationFramework::evaluateToCsv(const filesystem::path &csvPath,
                                             size_t runs,
                                             size_t sampleSize,
                                             const string &algorithmParams,
                                             double rseTheoretical,
                                             Args &&... ctorArgs) const {
        if (detail::scopeIsEmpty(runs, sampleSize)) return {};
        const auto evaluationContext = context();
        return modes::normal::evaluateToCsv<Algo>(
            evaluationContext,
            csvPath,
            algorithmParams,
            rseTheoretical,
            forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    vector<StreamingPointStats> EvaluationFramework::evaluateStreaming(size_t runs,
                                                                       size_t sampleSize,
                                                                       Args &&... ctorArgs) const {
        if (detail::scopeIsEmpty(runs, sampleSize)) return {};
        const auto evaluationContext = context();
        return modes::streaming::evaluate<Algo>(evaluationContext, forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    vector<StreamingPointStats> EvaluationFramework::evaluateStreamingToCsv(
        const filesystem::path &csvPath,
        size_t runs,
        size_t sampleSize,
        const string &algorithmParams,
        double rseTheoretical,
        Args &&... ctorArgs) const {
        if (detail::scopeIsEmpty(runs, sampleSize)) return {};
        const auto evaluationContext = context();
        return modes::streaming::evaluateToCsv<Algo>(
            evaluationContext,
            csvPath,
            algorithmParams,
            rseTheoretical,
            forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    vector<MergePairPoint> EvaluationFramework::evaluateMergePairs(size_t runs,
                                                                   size_t sampleSize,
                                                                   Args &&... ctorArgs) const {
        if (detail::scopeIsEmpty(runs, sampleSize)) return {};
        const auto evaluationContext = context();
        return modes::merge::evaluate<Algo>(evaluationContext, forward<Args>(ctorArgs)...);
    }

    template<typename Algo, typename... Args>
    MergePairStats EvaluationFramework::evaluateMergePairsToCsv(
        const filesystem::path &csvPath,
        size_t runs,
        size_t sampleSize,
        const string &algorithmParams,
        Args &&... ctorArgs) const {
        if (detail::scopeIsEmpty(runs, sampleSize)) return {};
        const auto evaluationContext = context();
        return modes::merge::evaluateToCsv<Algo>(
            evaluationContext,
            csvPath,
            algorithmParams,
            forward<Args>(ctorArgs)...);
    }
} // namespace satp::evaluation
