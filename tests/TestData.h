#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "satp/io/TextDatasetIO.h"

namespace satp::testdata {
    struct LoadedDataset {
        std::vector<std::uint32_t> values;
        std::size_t total = 0;
        std::size_t distinct = 0;
    };

    inline std::filesystem::path datasetPath() {
        return std::filesystem::path(__FILE__).parent_path() / "data" / "dataset_10k_1k.txt";
    }

    inline LoadedDataset loadDataset() {
        LoadedDataset out;
        const auto info = satp::io::loadTextDataset(datasetPath(), out.values);
        out.total = info.total_elements;
        out.distinct = info.distinct_elements;
        return out;
    }
} // namespace satp::testdata
