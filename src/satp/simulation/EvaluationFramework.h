#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "satp/io/BinaryDatasetIO.h"
#include "satp/simulation/Stats.h"

namespace satp::evaluation {
    class EvaluationFramework {
    public:
        static constexpr std::size_t DEFAULT_STREAMING_CHECKPOINTS = 200u;

        explicit EvaluationFramework(const std::filesystem::path &filePath);
        explicit EvaluationFramework(satp::io::BinaryDatasetIndex datasetIndex);

        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluate(std::size_t runs,
                                     std::size_t sampleSize,
                                     Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluateToCsv(const std::filesystem::path &csvPath,
                                          std::size_t runs,
                                          std::size_t sampleSize,
                                          const std::string &algorithmParams,
                                          double rseTheoretical,
                                          Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] std::vector<StreamingPointStats> evaluateStreaming(std::size_t runs,
                                                                         std::size_t sampleSize,
                                                                         Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] std::vector<StreamingPointStats> evaluateStreamingToCsv(
            const std::filesystem::path &csvPath,
            std::size_t runs,
            std::size_t sampleSize,
            const std::string &algorithmParams,
            double rseTheoretical,
            Args &&... ctorArgs) const;

        [[nodiscard]] std::size_t getNumElementiDistintiEffettivi() const noexcept;

    private:
        struct EvaluationScope {
            std::size_t runs = 0;
            std::size_t sampleSize = 0;
        };

        [[nodiscard]] EvaluationScope datasetScope() const noexcept;

        template<typename Algo, typename... Args>
        [[nodiscard]] Stats evaluateFromBinary(Args &&... ctorArgs) const;

        template<typename Algo, typename... Args>
        [[nodiscard]] std::vector<StreamingPointStats> evaluateStreamingFromBinary(Args &&... ctorArgs) const;

        satp::io::BinaryDatasetIndex binaryDataset;
        std::size_t numElementiDistintiEffettivi = 0;
        std::uint32_t seed = 0;
    };
} // namespace satp::evaluation

#include "satp/simulation/EvaluationFramework.tpp"
