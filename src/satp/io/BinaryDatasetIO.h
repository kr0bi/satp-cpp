#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <zlib.h>

using namespace std;

namespace satp::io {
    struct BinaryPartitionEntry {
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

    struct BinaryDatasetInfo {
        size_t elements_per_partition = 0;
        size_t distinct_per_partition = 0;
        uint32_t seed = 0;
        size_t partition_count = 0;
    };

    struct BinaryDatasetIndex {
        filesystem::path path;
        BinaryDatasetInfo info;
        vector<BinaryPartitionEntry> partitions;
    };

    namespace detail {
        constexpr array<char, 8> MAGIC = {'S', 'A', 'T', 'P', 'D', 'B', 'N', '2'};
        constexpr uint32_t VERSION = 2u;
        constexpr uint32_t ENCODING_ZLIB_U32_LE = 1u;
        constexpr uint32_t ENCODING_ZLIB_BITSET_LE = 2u;
        constexpr size_t HEADER_SIZE = 44u; // <8sIQQQQ
        constexpr size_t ENTRY_SIZE = 60u;  // <QQQQQQIII

        inline uint32_t readU32LE(const uint8_t *p) {
            return static_cast<uint32_t>(p[0])
                   | (static_cast<uint32_t>(p[1]) << 8u)
                   | (static_cast<uint32_t>(p[2]) << 16u)
                   | (static_cast<uint32_t>(p[3]) << 24u);
        }

        inline uint64_t readU64LE(const uint8_t *p) {
            return static_cast<uint64_t>(p[0])
                   | (static_cast<uint64_t>(p[1]) << 8u)
                   | (static_cast<uint64_t>(p[2]) << 16u)
                   | (static_cast<uint64_t>(p[3]) << 24u)
                   | (static_cast<uint64_t>(p[4]) << 32u)
                   | (static_cast<uint64_t>(p[5]) << 40u)
                   | (static_cast<uint64_t>(p[6]) << 48u)
                   | (static_cast<uint64_t>(p[7]) << 56u);
        }

        inline size_t toSizeTChecked(uint64_t value, const string &field) {
            if (value > static_cast<uint64_t>(numeric_limits<size_t>::max())) {
                throw runtime_error("Binary dataset field '" + field + "' is too large for size_t");
            }
            return static_cast<size_t>(value);
        }

        inline streamoff toStreamoffChecked(uint64_t value, const string &field) {
            if (value > static_cast<uint64_t>(numeric_limits<streamoff>::max())) {
                throw runtime_error("Binary dataset field '" + field + "' is too large for streamoff");
            }
            return static_cast<streamoff>(value);
        }

        inline void readExact(ifstream &in, uint8_t *dst, size_t n, const char *error) {
            in.read(reinterpret_cast<char *>(dst), static_cast<streamsize>(n));
            if (in.gcount() != static_cast<streamsize>(n)) {
                throw runtime_error(error);
            }
        }

        inline void decompressZlibBlock(const vector<uint8_t> &compressed,
                                        size_t expectedBytes,
                                        vector<uint8_t> &decompressed,
                                        const char *error) {
            decompressed.resize(expectedBytes);
            uLongf decompressedLen = static_cast<uLongf>(decompressed.size());
            const int rc = ::uncompress(
                reinterpret_cast<Bytef *>(decompressed.data()),
                &decompressedLen,
                reinterpret_cast<const Bytef *>(compressed.data()),
                static_cast<uLong>(compressed.size()));
            if (rc != Z_OK || decompressedLen != static_cast<uLongf>(expectedBytes)) {
                throw runtime_error(error);
            }
        }
    } // namespace detail

    inline BinaryDatasetIndex indexBinaryDataset(const filesystem::path &path) {
        ifstream in(path, ios::binary);
        if (!in) throw runtime_error("Cannot open binary dataset file");

        in.seekg(0, ios::end);
        const auto endPos = in.tellg();
        if (endPos < 0) throw runtime_error("Cannot determine binary dataset size");
        const auto fileSize = static_cast<uint64_t>(endPos);
        in.seekg(0, ios::beg);

        if (fileSize < detail::HEADER_SIZE) {
            throw runtime_error("Invalid binary dataset: file too small for header");
        }

        array<uint8_t, detail::HEADER_SIZE> header{};
        detail::readExact(in, header.data(), header.size(), "Invalid binary dataset header");

        if (!equal(detail::MAGIC.begin(), detail::MAGIC.end(), reinterpret_cast<const char *>(header.data()))) {
            throw runtime_error("Invalid binary dataset: bad magic");
        }

        const uint32_t version = detail::readU32LE(header.data() + 8u);
        if (version != detail::VERSION) {
            throw runtime_error("Invalid binary dataset: unsupported version");
        }

        const uint64_t nRaw = detail::readU64LE(header.data() + 12u);
        const uint64_t dRaw = detail::readU64LE(header.data() + 20u);
        const uint64_t pRaw = detail::readU64LE(header.data() + 28u);
        const uint64_t seedRaw = detail::readU64LE(header.data() + 36u);

        BinaryDatasetIndex index;
        index.path = path;
        index.info.elements_per_partition = detail::toSizeTChecked(nRaw, "n");
        index.info.distinct_per_partition = detail::toSizeTChecked(dRaw, "d");
        index.info.partition_count = detail::toSizeTChecked(pRaw, "p");
        if (seedRaw > static_cast<uint64_t>(numeric_limits<uint32_t>::max())) {
            throw runtime_error("Binary dataset seed out of uint32_t range");
        }
        index.info.seed = static_cast<uint32_t>(seedRaw);

        if (index.info.distinct_per_partition > index.info.elements_per_partition) {
            throw runtime_error("Invalid binary dataset: distinct exceeds nOfElements");
        }

        const uint64_t tableBytes = static_cast<uint64_t>(index.info.partition_count) * detail::ENTRY_SIZE;
        const uint64_t minSize = detail::HEADER_SIZE + tableBytes;
        if (fileSize < minSize) {
            throw runtime_error("Invalid binary dataset: file too small for partition table");
        }

        index.partitions.reserve(index.info.partition_count);
        for (size_t i = 0; i < index.info.partition_count; ++i) {
            array<uint8_t, detail::ENTRY_SIZE> rawEntry{};
            detail::readExact(in, rawEntry.data(), rawEntry.size(), "Invalid binary partition table entry");

            BinaryPartitionEntry entry;
            entry.values_offset = detail::readU64LE(rawEntry.data());
            entry.values_byte_size = detail::readU64LE(rawEntry.data() + 8u);
            entry.truth_offset = detail::readU64LE(rawEntry.data() + 16u);
            entry.truth_byte_size = detail::readU64LE(rawEntry.data() + 24u);
            const uint64_t nPartRaw = detail::readU64LE(rawEntry.data() + 32u);
            const uint64_t dPartRaw = detail::readU64LE(rawEntry.data() + 40u);
            entry.values_encoding = detail::readU32LE(rawEntry.data() + 48u);
            entry.truth_encoding = detail::readU32LE(rawEntry.data() + 52u);
            entry.reserved = detail::readU32LE(rawEntry.data() + 56u);

            entry.elements = detail::toSizeTChecked(nPartRaw, "entry.n");
            entry.distinct = detail::toSizeTChecked(dPartRaw, "entry.d");

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

    inline void loadBinaryPartition(const BinaryDatasetIndex &index,
                                    size_t partitionIndex,
                                    vector<uint32_t> &out) {
        if (partitionIndex >= index.partitions.size()) {
            throw runtime_error("Requested partition index out of range");
        }
        const auto &entry = index.partitions[partitionIndex];

        out.clear();
        out.resize(entry.elements);
        if (entry.elements == 0) {
            return;
        }

        ifstream in(index.path, ios::binary);
        if (!in) throw runtime_error("Cannot open binary dataset file");

        const auto offset = detail::toStreamoffChecked(entry.values_offset, "entry.values_offset");
        in.seekg(offset, ios::beg);
        if (!in) throw runtime_error("Cannot seek binary dataset partition");

        vector<uint8_t> compressed(detail::toSizeTChecked(entry.values_byte_size, "entry.values_byte_size"));
        detail::readExact(in, compressed.data(), compressed.size(), "Cannot read binary dataset partition payload");

        const uint64_t expectedBytesU64 = static_cast<uint64_t>(entry.elements) * 4ull;
        const size_t expectedBytes = detail::toSizeTChecked(expectedBytesU64, "partition.uncompressed_size");

        vector<uint8_t> decompressed;
        detail::decompressZlibBlock(compressed, expectedBytes, decompressed,
                                    "Cannot decompress binary dataset partition");

        for (size_t i = 0; i < entry.elements; ++i) {
            const auto *p = decompressed.data() + (i * 4u);
            out[i] = detail::readU32LE(p);
        }
    }

    inline void loadBinaryPartitionTruthBits(const BinaryDatasetIndex &index,
                                             size_t partitionIndex,
                                             vector<uint8_t> &outTruthBits) {
        if (partitionIndex >= index.partitions.size()) {
            throw runtime_error("Requested partition index out of range");
        }
        const auto &entry = index.partitions[partitionIndex];

        const size_t expectedBytes = (entry.elements + 7u) / 8u;
        outTruthBits.clear();
        outTruthBits.resize(expectedBytes);
        if (expectedBytes == 0) {
            return;
        }

        ifstream in(index.path, ios::binary);
        if (!in) throw runtime_error("Cannot open binary dataset file");

        const auto offset = detail::toStreamoffChecked(entry.truth_offset, "entry.truth_offset");
        in.seekg(offset, ios::beg);
        if (!in) throw runtime_error("Cannot seek binary dataset truth partition");

        vector<uint8_t> compressed(detail::toSizeTChecked(entry.truth_byte_size, "entry.truth_byte_size"));
        detail::readExact(in, compressed.data(), compressed.size(), "Cannot read binary dataset truth payload");

        vector<uint8_t> decompressed;
        detail::decompressZlibBlock(compressed, expectedBytes, decompressed,
                                    "Cannot decompress binary dataset truth");
        outTruthBits = move(decompressed);
    }

    class BinaryDatasetPartitionReader {
    public:
        explicit BinaryDatasetPartitionReader(const BinaryDatasetIndex &index)
            : index(index), in(index.path, ios::binary) {
            if (!in) {
                throw runtime_error("Cannot open binary dataset file");
            }
        }

        void load(size_t partitionIndex, vector<uint32_t> &out) {
            if (partitionIndex >= index.partitions.size()) {
                throw runtime_error("Requested partition index out of range");
            }
            const auto &entry = index.partitions[partitionIndex];

            out.clear();
            out.resize(entry.elements);
            if (entry.elements == 0) {
                return;
            }

            const auto offset = detail::toStreamoffChecked(entry.values_offset, "entry.values_offset");
            in.clear();
            in.seekg(offset, ios::beg);
            if (!in) throw runtime_error("Cannot seek binary dataset partition");

            compressed.resize(detail::toSizeTChecked(entry.values_byte_size, "entry.values_byte_size"));
            detail::readExact(in, compressed.data(), compressed.size(), "Cannot read binary dataset partition payload");

            const uint64_t expectedBytesU64 = static_cast<uint64_t>(entry.elements) * 4ull;
            const size_t expectedBytes = detail::toSizeTChecked(expectedBytesU64, "partition.uncompressed_size");

            detail::decompressZlibBlock(compressed, expectedBytes, decompressed,
                                        "Cannot decompress binary dataset partition");

            for (size_t i = 0; i < entry.elements; ++i) {
                const auto *p = decompressed.data() + (i * 4u);
                out[i] = detail::readU32LE(p);
            }
        }

        void loadWithTruthBits(size_t partitionIndex,
                               vector<uint32_t> &outValues,
                               vector<uint8_t> &outTruthBits) {
            if (partitionIndex >= index.partitions.size()) {
                throw runtime_error("Requested partition index out of range");
            }
            const auto &entry = index.partitions[partitionIndex];

            load(partitionIndex, outValues);

            const auto truthOffset = detail::toStreamoffChecked(entry.truth_offset, "entry.truth_offset");
            in.clear();
            in.seekg(truthOffset, ios::beg);
            if (!in) throw runtime_error("Cannot seek binary dataset truth partition");

            compressedTruth.resize(detail::toSizeTChecked(entry.truth_byte_size, "entry.truth_byte_size"));
            detail::readExact(in, compressedTruth.data(), compressedTruth.size(), "Cannot read binary dataset truth payload");

            const size_t expectedTruthBytes = (entry.elements + 7u) / 8u;
            detail::decompressZlibBlock(compressedTruth, expectedTruthBytes, decompressedTruth,
                                        "Cannot decompress binary dataset truth");
            outTruthBits = decompressedTruth;
        }

    private:
        const BinaryDatasetIndex &index;
        ifstream in;
        vector<uint8_t> compressed;
        vector<uint8_t> decompressed;
        vector<uint8_t> compressedTruth;
        vector<uint8_t> decompressedTruth;
    };
} // namespace satp::io
