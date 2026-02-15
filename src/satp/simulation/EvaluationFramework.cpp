#include "satp/simulation/EvaluationFramework.h"

#include <utility>

namespace satp::evaluation {
    EvaluationFramework::EvaluationFramework(const std::filesystem::path &filePath)
        : EvaluationFramework(satp::io::indexBinaryDataset(filePath)) {
    }

    EvaluationFramework::EvaluationFramework(satp::io::BinaryDatasetIndex datasetIndex)
        : binaryDataset(std::move(datasetIndex)),
          numElementiDistintiEffettivi(binaryDataset.info.distinct_per_partition),
          seed(binaryDataset.info.seed) {
    }

    std::size_t EvaluationFramework::getNumElementiDistintiEffettivi() const noexcept {
        return numElementiDistintiEffettivi;
    }

    EvaluationFramework::EvaluationScope EvaluationFramework::datasetScope() const noexcept {
        return {
            binaryDataset.info.partition_count,
            binaryDataset.info.elements_per_partition
        };
    }
} // namespace satp::evaluation
