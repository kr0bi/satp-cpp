#pragma once

#include <filesystem>
#include <vector>

#include "satp/dataset/detail/DatasetTypes.h"

using namespace std;

namespace satp::dataset {
    [[nodiscard]] DatasetIndex indexBinaryDataset(const filesystem::path &path);

    void loadBinaryPartition(const DatasetIndex &index,
                             size_t partitionIndex,
                             vector<uint32_t> &out);

    void loadBinaryPartitionTruthBits(const DatasetIndex &index,
                                      size_t partitionIndex,
                                      vector<uint8_t> &outTruthBits);
} // namespace satp::dataset
