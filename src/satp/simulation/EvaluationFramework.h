#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "satp/hashing/HashFunction.h"
#include "satp/io/BinaryDatasetIO.h"
#include "satp/simulation/Stats.h"
#include "satp/simulation/evaluationFramework/Context.h"

namespace satp::evaluation {
    namespace modes::normal {
        template<typename Algo, typename... Args>
        Stats evaluate(const detail::EvaluationContext &context, Args &&... ctorArgs);

        template<typename Algo, typename... Args>
        Stats evaluateToCsv(const detail::EvaluationContext &context,
                            const filesystem::path &csvPath,
                            const string &algorithmParams,
                            double rseTheoretical,
                            Args &&... ctorArgs);
    } // namespace modes::normal

    namespace modes::streaming {
        template<typename Algo, typename... Args>
        vector<StreamingPointStats> evaluate(const detail::EvaluationContext &context, Args &&... ctorArgs);

        template<typename Algo, typename... Args>
        vector<StreamingPointStats> evaluateToCsv(const detail::EvaluationContext &context,
                                                  const filesystem::path &csvPath,
                                                  const string &algorithmParams,
                                                  double rseTheoretical,
                                                  Args &&... ctorArgs);
    } // namespace modes::streaming

    namespace modes::merge {
        template<typename Algo, typename... Args>
        vector<MergePairPoint> evaluate(const detail::EvaluationContext &context, Args &&... ctorArgs);

        template<typename Algo, typename... Args>
        MergePairStats evaluateToCsv(const detail::EvaluationContext &context,
                                     const filesystem::path &csvPath,
                                     const string &algorithmParams,
                                     Args &&... ctorArgs);
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
        [[nodiscard]] Stats evaluate(size_t runs,
                                     size_t sampleSize,
                                     Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluateToCsv(const filesystem::path &csvPath,
                                          size_t runs,
                                          size_t sampleSize,
                                          const string &algorithmParams,
                                          double rseTheoretical,
                                          Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<StreamingPointStats> evaluateStreaming(size_t runs,
                                                                         size_t sampleSize,
                                                                         Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<StreamingPointStats> evaluateStreamingToCsv(
            const filesystem::path &csvPath,
            size_t runs,
            size_t sampleSize,
            const string &algorithmParams,
            double rseTheoretical,
            Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<MergePairPoint> evaluateMergePairs(size_t runs,
                                                                     size_t sampleSize,
                                                                     Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] MergePairStats evaluateMergePairsToCsv(
            const filesystem::path &csvPath,
            size_t runs,
            size_t sampleSize,
            const string &algorithmParams,
            Args &&... ctorArgs) const;

        [[nodiscard]] size_t getNumElementiDistintiEffettivi() const noexcept;

    private:
        [[nodiscard]] detail::EvaluationContext context() const;

        io::BinaryDatasetIndex binaryDataset;
        size_t numElementiDistintiEffettivi = 0;
        uint32_t seed = 0;
        unique_ptr<hashing::HashFunction> hashFunction;
    };
} // namespace satp::evaluation

#include "satp/simulation/evaluationFramework/Detail.h"
#include "satp/simulation/evaluationFramework/modes/normal/Core.tpp"
#include "satp/simulation/evaluationFramework/modes/normal/Csv.tpp"
#include "satp/simulation/evaluationFramework/modes/streaming/Core.tpp"
#include "satp/simulation/evaluationFramework/modes/streaming/Csv.tpp"
#include "satp/simulation/evaluationFramework/modes/merge/Core.tpp"
#include "satp/simulation/evaluationFramework/modes/merge/Csv.tpp"
#include "satp/simulation/evaluationFramework/Facade.tpp"

using namespace std;
