#pragma once

#include <vector>

#include "satp/dataset/detail/DatasetTypes.h"
#include "satp/dataset/detail/binary/Endian.h"
#include "satp/dataset/detail/binary/FileIO.h"

using namespace std;

namespace satp::dataset::detail {
    [[nodiscard]] inline const PartitionEntry &partitionEntryOrThrow(const DatasetIndex &index,
                                                                     size_t partitionIndex) {
        if (partitionIndex >= index.partitions.size()) {
            throw runtime_error("Requested partition index out of range");
        }
        return index.partitions[partitionIndex];
    }

    inline void loadValuesInto(ifstream &input,
                               const PartitionEntry &entry,
                               vector<uint8_t> &compressed,
                               vector<uint8_t> &decompressed,
                               vector<uint32_t> &out) {
        out.assign(entry.elements, 0u);
        if (entry.elements == 0) return;

        seekChecked(input, entry.values_offset, "entry.values_offset", "Cannot seek binary dataset partition");
        compressed.resize(toSizeTChecked(entry.values_byte_size, "entry.values_byte_size"));
        readExact(input, compressed.data(), compressed.size(), "Cannot read binary dataset partition payload");

        const size_t expectedBytes = toSizeTChecked(static_cast<uint64_t>(entry.elements) * 4ull,
                                                    "partition.uncompressed_size");
        decompressZlibBlock(compressed, expectedBytes, decompressed, "Cannot decompress binary dataset partition");

        for (size_t i = 0; i < entry.elements; ++i) {
            out[i] = readU32LE(decompressed.data() + (i * 4u));
        }
    }

    inline void loadTruthBitsInto(ifstream &input,
                                  const PartitionEntry &entry,
                                  vector<uint8_t> &compressed,
                                  vector<uint8_t> &decompressed,
                                  vector<uint8_t> &outTruthBits) {
        const size_t expectedTruthBytes = (entry.elements + 7u) / 8u;
        outTruthBits.assign(expectedTruthBytes, 0u);
        if (expectedTruthBytes == 0) return;

        seekChecked(input, entry.truth_offset, "entry.truth_offset", "Cannot seek binary dataset truth partition");
        compressed.resize(toSizeTChecked(entry.truth_byte_size, "entry.truth_byte_size"));
        readExact(input, compressed.data(), compressed.size(), "Cannot read binary dataset truth payload");
        decompressZlibBlock(compressed, expectedTruthBytes, decompressed, "Cannot decompress binary dataset truth");
        outTruthBits = decompressed;
    }
} // namespace satp::dataset::detail
