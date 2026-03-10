#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "satp/dataset/Dataset.h"

using namespace std;

namespace satp::testdata {
    struct LoadedDataset {
        vector<uint32_t> values;
        size_t elements_per_partition = 0;
        size_t distinct = 0;
        size_t partition_count = 0;
        uint32_t seed = 0;
    };

    inline filesystem::path datasetPath() {
        return filesystem::path(__FILE__).parent_path() / "data" / "dataset_n_10000_d_5000_p_3_s_5489.bin";
    }

    inline LoadedDataset loadDataset() {
        LoadedDataset out;
        const auto index = satp::dataset::indexBinaryDataset(datasetPath());
        satp::dataset::loadBinaryPartition(index, 0, out.values);
        out.elements_per_partition = index.info.elements_per_partition;
        out.distinct = index.info.distinct_per_partition;
        out.partition_count = index.info.partition_count;
        out.seed = index.info.seed;
        return out;
    }

    inline vector<uint32_t> loadPartition(const size_t partitionIndex) {
        const auto index = satp::dataset::indexBinaryDataset(datasetPath());
        vector<uint32_t> out;
        satp::dataset::loadBinaryPartition(index, partitionIndex, out);
        return out;
    }
} // namespace satp::testdata
