#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "satp/hashing/HashFunction.h"
#include "satp/io/BinaryDatasetIO.h"
#include "satp/simulation/framework/Context.h"
#include "satp/simulation/framework/EvaluationMetadata.h"
#include "satp/simulation/framework/ProgressCallbacks.h"
#include "satp/simulation/metrics/Stats.h"

namespace satp::evaluation {
    namespace modes::streaming {
        template<typename Algo, typename... Args>
        vector<StreamingPointStats> evaluate(const detail::EvaluationContext &context, Args &&... ctorArgs);
    } // namespace modes::streaming

    namespace modes::merge {
        template<typename Algo, typename... Args>
        vector<MergePairPoint> evaluate(const detail::EvaluationContext &context, Args &&... ctorArgs);
    } // namespace modes::merge

    class EvaluationFramework {
    public:
        static constexpr size_t DEFAULT_STREAMING_CHECKPOINTS = 200u;

        explicit EvaluationFramework(
            const filesystem::path &filePath,
            unique_ptr<hashing::HashFunction> hashFunction);
        explicit EvaluationFramework(
            io::BinaryDatasetIndex datasetIndex,
            unique_ptr<hashing::HashFunction> hashFunction);

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<StreamingPointStats> evaluateStreaming(Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<StreamingPointStats> evaluateStreaming(const ProgressCallbacks &progress,
                                                                    Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<MergePairPoint> evaluateMergePairs(Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<MergePairPoint> evaluateMergePairs(const ProgressCallbacks &progress,
                                                                Args &&... ctorArgs) const;

        [[nodiscard]] const EvaluationMetadata &metadata() const noexcept;

    private:
        [[nodiscard]] detail::EvaluationContext context(const ProgressCallbacks *progress = nullptr) const;

        io::BinaryDatasetIndex binaryDataset;
        EvaluationMetadata metadata_;
        unique_ptr<hashing::HashFunction> hashFunction;
    };
} // namespace satp::evaluation

#include "satp/simulation/framework/Facade.tpp"
#include "satp/simulation/merge/MergeEvaluation.tpp"
#include "satp/simulation/streaming/StreamingEvaluation.tpp"

using namespace std;
