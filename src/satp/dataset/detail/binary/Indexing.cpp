#include "satp/dataset/detail/DatasetAccess.h"

#include <algorithm>
#include <array>
#include <fstream>

#include "satp/dataset/detail/binary/Endian.h"
#include "satp/dataset/detail/binary/FileIO.h"
#include "satp/dataset/detail/binary/Format.h"

using namespace std;

namespace satp::dataset {
    DatasetIndex indexBinaryDataset(const filesystem::path &path) {
        ifstream input(path, ios::binary);
        if (!input) throw runtime_error("Cannot open binary dataset file");

        input.seekg(0, ios::end);
        const auto endPos = input.tellg();
        if (endPos < 0) throw runtime_error("Cannot determine binary dataset size");
        const auto fileSize = static_cast<uint64_t>(endPos);
        input.seekg(0, ios::beg);
        if (fileSize < detail::HEADER_SIZE) throw runtime_error("Invalid binary dataset: file too small for header");

        array<uint8_t, detail::HEADER_SIZE> header{};
        detail::readExact(input, header.data(), header.size(), "Invalid binary dataset header");
        if (!equal(detail::MAGIC.begin(), detail::MAGIC.end(), reinterpret_cast<const char *>(header.data()))) {
            throw runtime_error("Invalid binary dataset: bad magic");
        }

        const uint32_t version = detail::readU32LE(header.data() + 8u);
        if (version != detail::VERSION) throw runtime_error("Invalid binary dataset: unsupported version");

        DatasetIndex index;
        index.path = path;
        index.info.elements_per_partition = detail::toSizeTChecked(detail::readU64LE(header.data() + 12u), "n");
        index.info.distinct_per_partition = detail::toSizeTChecked(detail::readU64LE(header.data() + 20u), "d");
        index.info.partition_count = detail::toSizeTChecked(detail::readU64LE(header.data() + 28u), "p");

        const uint64_t seed = detail::readU64LE(header.data() + 36u);
        if (seed > static_cast<uint64_t>(numeric_limits<uint32_t>::max())) {
            throw runtime_error("Binary dataset seed out of uint32_t range");
        }
        index.info.seed = static_cast<uint32_t>(seed);
        if (index.info.distinct_per_partition > index.info.elements_per_partition) {
            throw runtime_error("Invalid binary dataset: distinct exceeds nOfElements");
        }

        const uint64_t tableBytes = static_cast<uint64_t>(index.info.partition_count) * detail::ENTRY_SIZE;
        if (fileSize < detail::HEADER_SIZE + tableBytes) {
            throw runtime_error("Invalid binary dataset: file too small for partition table");
        }

        index.partitions.reserve(index.info.partition_count);
        for (size_t i = 0; i < index.info.partition_count; ++i) {
            array<uint8_t, detail::ENTRY_SIZE> rawEntry{};
            detail::readExact(input, rawEntry.data(), rawEntry.size(), "Invalid binary partition table entry");

            PartitionEntry entry;
            entry.values_offset = detail::readU64LE(rawEntry.data());
            entry.values_byte_size = detail::readU64LE(rawEntry.data() + 8u);
            entry.truth_offset = detail::readU64LE(rawEntry.data() + 16u);
            entry.truth_byte_size = detail::readU64LE(rawEntry.data() + 24u);
            entry.elements = detail::toSizeTChecked(detail::readU64LE(rawEntry.data() + 32u), "entry.n");
            entry.distinct = detail::toSizeTChecked(detail::readU64LE(rawEntry.data() + 40u), "entry.d");
            entry.values_encoding = detail::readU32LE(rawEntry.data() + 48u);
            entry.truth_encoding = detail::readU32LE(rawEntry.data() + 52u);
            entry.reserved = detail::readU32LE(rawEntry.data() + 56u);

            if (entry.elements != index.info.elements_per_partition || entry.distinct != index.info.distinct_per_partition) {
                throw runtime_error("Invalid binary dataset: partition metadata mismatch");
            }
            if (entry.values_encoding != detail::ENCODING_ZLIB_U32_LE) {
                throw runtime_error("Invalid binary dataset: unsupported values encoding");
            }
            if (entry.truth_encoding != detail::ENCODING_ZLIB_BITSET_LE) {
                throw runtime_error("Invalid binary dataset: unsupported truth encoding");
            }
            if (entry.values_offset > fileSize || entry.values_byte_size > fileSize ||
                entry.values_offset + entry.values_byte_size > fileSize) {
                throw runtime_error("Invalid binary dataset: values range out of bounds");
            }
            if (entry.truth_offset > fileSize || entry.truth_byte_size > fileSize ||
                entry.truth_offset + entry.truth_byte_size > fileSize) {
                throw runtime_error("Invalid binary dataset: truth range out of bounds");
            }
            index.partitions.push_back(entry);
        }

        return index;
    }
} // namespace satp::dataset
