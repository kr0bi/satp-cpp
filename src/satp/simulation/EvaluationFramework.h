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

namespace satp::evaluation {
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
        struct EvaluationScope {
            size_t runs = 0;
            size_t sampleSize = 0;
        };

        [[nodiscard]] EvaluationScope datasetScope() const noexcept;

        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluateFromBinary(Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] vector<StreamingPointStats> evaluateStreamingFromBinary(Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] Algo makeAlgo(Args &&... ctorArgs) const;

        io::BinaryDatasetIndex binaryDataset;
        size_t numElementiDistintiEffettivi = 0;
        uint32_t seed = 0;
        unique_ptr<hashing::HashFunction> hashFunction;
    };
} // namespace satp::evaluation

#include "satp/simulation/EvaluationFramework.tpp"

using namespace std;
