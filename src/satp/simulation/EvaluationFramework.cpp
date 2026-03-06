#include "satp/simulation/EvaluationFramework.h"

#include <stdexcept>
#include <utility>

using namespace std;

namespace satp::evaluation {
    EvaluationFramework::EvaluationFramework(
        const filesystem::path &filePath,
        unique_ptr<hashing::HashFunction> hashFunction)
        : EvaluationFramework(io::indexBinaryDataset(filePath), std::move(hashFunction)) {
    }

    EvaluationFramework::EvaluationFramework(
        io::BinaryDatasetIndex datasetIndex,
        unique_ptr<hashing::HashFunction> hashFunction)
        : binaryDataset(std::move(datasetIndex)),
          metadata_{
              binaryDataset.info.partition_count,
              binaryDataset.info.elements_per_partition,
              binaryDataset.info.distinct_per_partition,
              binaryDataset.info.seed
          },
          hashFunction(std::move(hashFunction)) {
        if (this->hashFunction == nullptr) {
            throw invalid_argument("EvaluationFramework requires a non-null hash function");
        }
    }

    const EvaluationMetadata &EvaluationFramework::metadata() const noexcept {
        return metadata_;
    }

    detail::EvaluationContext EvaluationFramework::context(const ProgressCallbacks *progress) const {
        return {
            binaryDataset,
            metadata_,
            *hashFunction,
            progress
        };
    }
} // namespace satp::evaluation
