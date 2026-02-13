#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "satp/io/BinaryDatasetIO.h"

namespace satp::testdata {
    struct LoadedDataset {
        std::vector<std::uint32_t> values;
        std::size_t elements_per_partition = 0;
        std::size_t distinct = 0;
        std::size_t partition_count = 0;
        std::uint32_t seed = 0;
    };

    inline std::filesystem::path datasetPath() {
        return std::filesystem::path(__FILE__).parent_path() / "data" / "dataset_n_2000_d_1000_p_3_s_5489.bin";
    }

    inline LoadedDataset loadDataset() {
        LoadedDataset out;
        const auto index = satp::io::indexBinaryDataset(datasetPath());
        satp::io::loadBinaryPartition(index, 0, out.values);
        out.elements_per_partition = index.info.elements_per_partition;
        out.distinct = index.info.distinct_per_partition;
        out.partition_count = index.info.partition_count;
        out.seed = index.info.seed;
        return out;
    }
} // namespace satp::testdata
