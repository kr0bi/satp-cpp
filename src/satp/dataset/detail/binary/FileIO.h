#pragma once

#include <fstream>
#include <stdexcept>
#include <vector>

#include <zlib.h>

using namespace std;

namespace satp::dataset::detail {
    inline void readExact(ifstream &input, uint8_t *destination, size_t bytes, const char *error) {
        input.read(reinterpret_cast<char *>(destination), static_cast<streamsize>(bytes));
        if (input.gcount() != static_cast<streamsize>(bytes)) {
            throw runtime_error(error);
        }
    }

    inline void seekChecked(ifstream &input, uint64_t offset, const string &field, const char *error) {
        input.clear();
        input.seekg(toStreamoffChecked(offset, field), ios::beg);
        if (!input) {
            throw runtime_error(error);
        }
    }

    inline void decompressZlibBlock(const vector<uint8_t> &compressed,
                                    size_t expectedBytes,
                                    vector<uint8_t> &decompressed,
                                    const char *error) {
        decompressed.resize(expectedBytes);
        uLongf decompressedLen = static_cast<uLongf>(decompressed.size());
        const int rc = ::uncompress(reinterpret_cast<Bytef *>(decompressed.data()),
                                    &decompressedLen,
                                    reinterpret_cast<const Bytef *>(compressed.data()),
                                    static_cast<uLong>(compressed.size()));
        if (rc != Z_OK || decompressedLen != static_cast<uLongf>(expectedBytes)) {
            throw runtime_error(error);
        }
    }
} // namespace satp::dataset::detail

