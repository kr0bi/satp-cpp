#include "satp/dataset/detail/DatasetAccess.h"

#include "satp/dataset/detail/binary/PartitionIO.h"

using namespace std;

namespace satp::dataset {
    void loadBinaryPartition(const DatasetIndex &index,
                             size_t partitionIndex,
                             vector<uint32_t> &out) {
        const auto &entry = detail::partitionEntryOrThrow(index, partitionIndex);
        ifstream input(index.path, ios::binary);
        if (!input) throw runtime_error("Cannot open binary dataset file");
        vector<uint8_t> compressed;
        vector<uint8_t> decompressed;
        detail::loadValuesInto(input, entry, compressed, decompressed, out);
    }

    void loadBinaryPartitionTruthBits(const DatasetIndex &index,
                                      size_t partitionIndex,
                                      vector<uint8_t> &outTruthBits) {
        const auto &entry = detail::partitionEntryOrThrow(index, partitionIndex);
        ifstream input(index.path, ios::binary);
        if (!input) throw runtime_error("Cannot open binary dataset file");
        vector<uint8_t> compressed;
        vector<uint8_t> decompressed;
        detail::loadTruthBitsInto(input, entry, compressed, decompressed, outTruthBits);
    }

    PartitionReader::PartitionReader(const DatasetIndex &index)
        : index_(index), input_(index.path, ios::binary) {
        if (!input_) throw runtime_error("Cannot open binary dataset file");
    }

    void PartitionReader::load(size_t partitionIndex, vector<uint32_t> &out) {
        detail::loadValuesInto(input_,
                               detail::partitionEntryOrThrow(index_, partitionIndex),
                               compressedValues_,
                               decompressedValues_,
                               out);
    }

    void PartitionReader::loadWithTruthBits(size_t partitionIndex,
                                            vector<uint32_t> &outValues,
                                            vector<uint8_t> &outTruthBits) {
        const auto &entry = detail::partitionEntryOrThrow(index_, partitionIndex);
        load(partitionIndex, outValues);
        detail::loadTruthBitsInto(input_, entry, compressedTruth_, decompressedTruth_, outTruthBits);
    }
} // namespace satp::dataset
