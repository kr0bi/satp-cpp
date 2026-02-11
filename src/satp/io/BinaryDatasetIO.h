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

namespace satp::io {
    struct BinaryPartitionEntry {
        std::uint64_t offset = 0;
        std::uint64_t byte_size = 0;
        std::size_t elements = 0;
        std::size_t distinct = 0;
        std::uint32_t encoding = 0;
        std::uint32_t reserved = 0;
    };

    struct BinaryDatasetInfo {
        std::size_t elements_per_partition = 0;
        std::size_t distinct_per_partition = 0;
        std::uint32_t seed = 0;
        std::size_t partition_count = 0;
    };

    struct BinaryDatasetIndex {
        std::filesystem::path path;
        BinaryDatasetInfo info;
        std::vector<BinaryPartitionEntry> partitions;
    };

    namespace detail {
        constexpr std::array<char, 8> MAGIC = {'S', 'A', 'T', 'P', 'D', 'B', 'N', '1'};
        constexpr std::uint32_t VERSION = 1u;
        constexpr std::uint32_t ENCODING_ZLIB_U32_LE = 1u;
        constexpr std::size_t HEADER_SIZE = 44u; // <8sIQQQQ
        constexpr std::size_t ENTRY_SIZE = 40u;  // <QQQQII

        inline std::uint32_t readU32LE(const std::uint8_t *p) {
            return static_cast<std::uint32_t>(p[0])
                   | (static_cast<std::uint32_t>(p[1]) << 8u)
                   | (static_cast<std::uint32_t>(p[2]) << 16u)
                   | (static_cast<std::uint32_t>(p[3]) << 24u);
        }

        inline std::uint64_t readU64LE(const std::uint8_t *p) {
            return static_cast<std::uint64_t>(p[0])
                   | (static_cast<std::uint64_t>(p[1]) << 8u)
                   | (static_cast<std::uint64_t>(p[2]) << 16u)
                   | (static_cast<std::uint64_t>(p[3]) << 24u)
                   | (static_cast<std::uint64_t>(p[4]) << 32u)
                   | (static_cast<std::uint64_t>(p[5]) << 40u)
                   | (static_cast<std::uint64_t>(p[6]) << 48u)
                   | (static_cast<std::uint64_t>(p[7]) << 56u);
        }

        inline std::size_t toSizeTChecked(std::uint64_t value, const std::string &field) {
            if (value > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
                throw std::runtime_error("Binary dataset field '" + field + "' is too large for size_t");
            }
            return static_cast<std::size_t>(value);
        }

        inline std::streamoff toStreamoffChecked(std::uint64_t value, const std::string &field) {
            if (value > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
                throw std::runtime_error("Binary dataset field '" + field + "' is too large for streamoff");
            }
            return static_cast<std::streamoff>(value);
        }

        inline void readExact(std::ifstream &in, std::uint8_t *dst, std::size_t n, const char *error) {
            in.read(reinterpret_cast<char *>(dst), static_cast<std::streamsize>(n));
            if (in.gcount() != static_cast<std::streamsize>(n)) {
                throw std::runtime_error(error);
            }
        }
    } // namespace detail

    inline BinaryDatasetIndex indexBinaryDataset(const std::filesystem::path &path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open binary dataset file");

        in.seekg(0, std::ios::end);
        const auto endPos = in.tellg();
        if (endPos < 0) throw std::runtime_error("Cannot determine binary dataset size");
        const std::uint64_t fileSize = static_cast<std::uint64_t>(endPos);
        in.seekg(0, std::ios::beg);

        if (fileSize < detail::HEADER_SIZE) {
            throw std::runtime_error("Invalid binary dataset: file too small for header");
        }

        std::array<std::uint8_t, detail::HEADER_SIZE> header{};
        detail::readExact(in, header.data(), header.size(), "Invalid binary dataset header");

        if (!std::equal(detail::MAGIC.begin(), detail::MAGIC.end(), reinterpret_cast<const char *>(header.data()))) {
            throw std::runtime_error("Invalid binary dataset: bad magic");
        }

        const std::uint32_t version = detail::readU32LE(header.data() + 8u);
        if (version != detail::VERSION) {
            throw std::runtime_error("Invalid binary dataset: unsupported version");
        }

        const std::uint64_t nRaw = detail::readU64LE(header.data() + 12u);
        const std::uint64_t dRaw = detail::readU64LE(header.data() + 20u);
        const std::uint64_t pRaw = detail::readU64LE(header.data() + 28u);
        const std::uint64_t seedRaw = detail::readU64LE(header.data() + 36u);

        BinaryDatasetIndex index;
        index.path = path;
        index.info.elements_per_partition = detail::toSizeTChecked(nRaw, "n");
        index.info.distinct_per_partition = detail::toSizeTChecked(dRaw, "d");
        index.info.partition_count = detail::toSizeTChecked(pRaw, "p");
        if (seedRaw > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
            throw std::runtime_error("Binary dataset seed out of uint32_t range");
        }
        index.info.seed = static_cast<std::uint32_t>(seedRaw);

        if (index.info.distinct_per_partition > index.info.elements_per_partition) {
            throw std::runtime_error("Invalid binary dataset: distinct exceeds nOfElements");
        }

        const std::uint64_t tableBytes = static_cast<std::uint64_t>(index.info.partition_count) * detail::ENTRY_SIZE;
        const std::uint64_t minSize = detail::HEADER_SIZE + tableBytes;
        if (fileSize < minSize) {
            throw std::runtime_error("Invalid binary dataset: file too small for partition table");
        }

        index.partitions.reserve(index.info.partition_count);
        for (std::size_t i = 0; i < index.info.partition_count; ++i) {
            std::array<std::uint8_t, detail::ENTRY_SIZE> rawEntry{};
            detail::readExact(in, rawEntry.data(), rawEntry.size(), "Invalid binary partition table entry");

            BinaryPartitionEntry entry;
            entry.offset = detail::readU64LE(rawEntry.data());
            entry.byte_size = detail::readU64LE(rawEntry.data() + 8u);
            const std::uint64_t nPartRaw = detail::readU64LE(rawEntry.data() + 16u);
            const std::uint64_t dPartRaw = detail::readU64LE(rawEntry.data() + 24u);
            entry.encoding = detail::readU32LE(rawEntry.data() + 32u);
            entry.reserved = detail::readU32LE(rawEntry.data() + 36u);

            entry.elements = detail::toSizeTChecked(nPartRaw, "entry.n");
            entry.distinct = detail::toSizeTChecked(dPartRaw, "entry.d");

            if (entry.elements != index.info.elements_per_partition || entry.distinct != index.info.distinct_per_partition) {
                throw std::runtime_error("Invalid binary dataset: partition metadata mismatch");
            }
            if (entry.encoding != detail::ENCODING_ZLIB_U32_LE) {
                throw std::runtime_error("Invalid binary dataset: unsupported encoding");
            }
            if (entry.offset > fileSize || entry.byte_size > fileSize || entry.offset + entry.byte_size > fileSize) {
                throw std::runtime_error("Invalid binary dataset: partition range out of bounds");
            }

            index.partitions.push_back(entry);
        }

        return index;
    }

    inline void loadBinaryPartition(const BinaryDatasetIndex &index,
                                    std::size_t partitionIndex,
                                    std::vector<std::uint32_t> &out) {
        if (partitionIndex >= index.partitions.size()) {
            throw std::runtime_error("Requested partition index out of range");
        }
        const auto &entry = index.partitions[partitionIndex];

        out.clear();
        out.resize(entry.elements);
        if (entry.elements == 0) {
            return;
        }

        std::ifstream in(index.path, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open binary dataset file");

        const auto offset = detail::toStreamoffChecked(entry.offset, "entry.offset");
        in.seekg(offset, std::ios::beg);
        if (!in) throw std::runtime_error("Cannot seek binary dataset partition");

        std::vector<std::uint8_t> compressed(detail::toSizeTChecked(entry.byte_size, "entry.byte_size"));
        detail::readExact(in, compressed.data(), compressed.size(), "Cannot read binary dataset partition payload");

        const std::uint64_t expectedBytesU64 = static_cast<std::uint64_t>(entry.elements) * 4ull;
        const std::size_t expectedBytes = detail::toSizeTChecked(expectedBytesU64, "partition.uncompressed_size");

        std::vector<std::uint8_t> decompressed(expectedBytes);
        uLongf decompressedLen = static_cast<uLongf>(decompressed.size());
        const int rc = ::uncompress(
            reinterpret_cast<Bytef *>(decompressed.data()),
            &decompressedLen,
            reinterpret_cast<const Bytef *>(compressed.data()),
            static_cast<uLong>(compressed.size()));
        if (rc != Z_OK || decompressedLen != static_cast<uLongf>(expectedBytes)) {
            throw std::runtime_error("Cannot decompress binary dataset partition");
        }

        for (std::size_t i = 0; i < entry.elements; ++i) {
            const auto *p = decompressed.data() + (i * 4u);
            out[i] = detail::readU32LE(p);
        }
    }
} // namespace satp::io
