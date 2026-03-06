#include "satp/simulation/EvaluationFramework.h"

#include <stdexcept>
#include <utility>

using namespace std;

namespace satp::evaluation {
    EvaluationFramework::EvaluationFramework(
        const filesystem::path &filePath,
        unique_ptr<hashing::HashFunction> hashFunction)
        : EvaluationFramework(io::indexBinaryDataset(filePath), move(hashFunction)) {
    }

    EvaluationFramework::EvaluationFramework(
        io::BinaryDatasetIndex datasetIndex,
        unique_ptr<hashing::HashFunction> hashFunction)
        : binaryDataset(move(datasetIndex)),
          numElementiDistintiEffettivi(binaryDataset.info.distinct_per_partition),
          seed(binaryDataset.info.seed),
          hashFunction(move(hashFunction)) {
        if (this->hashFunction == nullptr) {
            throw invalid_argument("EvaluationFramework requires a non-null hash function");
        }
    }

    size_t EvaluationFramework::getNumElementiDistintiEffettivi() const noexcept {
        return numElementiDistintiEffettivi;
    }

    detail::EvaluationContext EvaluationFramework::context() const {
        return {
            binaryDataset,
            binaryDataset.info.partition_count,
            binaryDataset.info.elements_per_partition,
            numElementiDistintiEffettivi,
            seed,
            *hashFunction
        };
    }
} // namespace satp::evaluation
