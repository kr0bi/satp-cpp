#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace std;

namespace satp::dataset {
    struct PartitionEntry {
        uint64_t values_offset = 0;
        uint64_t values_byte_size = 0;
        uint64_t truth_offset = 0;
        uint64_t truth_byte_size = 0;
        size_t elements = 0;
        size_t distinct = 0;
        uint32_t values_encoding = 0;
        uint32_t truth_encoding = 0;
        uint32_t reserved = 0;
    };

    struct DatasetInfo {
        size_t elements_per_partition = 0;
        size_t distinct_per_partition = 0;
        uint32_t seed = 0;
        size_t partition_count = 0;
    };

    struct DatasetIndex {
        filesystem::path path;
        DatasetInfo info;
        vector<PartitionEntry> partitions;
    };

    class PartitionReader {
    public:
        explicit PartitionReader(const DatasetIndex &index);
        void load(size_t partitionIndex, vector<uint32_t> &out);
        void loadWithTruthBits(size_t partitionIndex,
                               vector<uint32_t> &outValues,
                               vector<uint8_t> &outTruthBits);

    private:
        const DatasetIndex &index_;
        ifstream input_;
        vector<uint8_t> compressedValues_;
        vector<uint8_t> decompressedValues_;
        vector<uint8_t> compressedTruth_;
        vector<uint8_t> decompressedTruth_;
    };
} // namespace satp::dataset

