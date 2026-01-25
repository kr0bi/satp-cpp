#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace satp::io {
    struct TextDatasetInfo {
        std::size_t total_elements = 0;
        std::size_t distinct_elements = 0;
    };

    inline TextDatasetInfo loadTextDataset(const std::filesystem::path &path,
                                           std::vector<std::uint32_t> &data) {
        std::ifstream in(path);
        if (!in) throw std::runtime_error("Cannot open dataset file");

        std::uint64_t total = 0;
        std::uint64_t distinct = 0;
        if (!(in >> total >> distinct))
            throw std::runtime_error("Dataset header must be: <total> <distinct>");
        if (distinct > total)
            throw std::runtime_error("Distinct count exceeds total elements");
        if (total > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
            throw std::runtime_error("Dataset too large for this platform");

        data.clear();
        data.reserve(static_cast<std::size_t>(total));

        for (std::uint64_t i = 0; i < total; ++i) {
            std::uint64_t value = 0;
            if (!(in >> value))
                throw std::runtime_error("Dataset ended before reading all elements");
            if (value > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()))
                throw std::runtime_error("Dataset value out of uint32_t range");
            data.push_back(static_cast<std::uint32_t>(value));
        }

        std::uint64_t extra = 0;
        if (in >> extra)
            throw std::runtime_error("Dataset has more values than declared");

        return {static_cast<std::size_t>(total), static_cast<std::size_t>(distinct)};
    }
} // namespace satp::io
