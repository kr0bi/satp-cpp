#pragma once
#include <string>
#include <format>

namespace satp::io {
    /**
     * Genera un nome file univoco a partire dai parametri del benchmark.
     * Esempio:
     *   dataset_N1e8_H1e9_S1e6_R30.bin
     */
    inline std::string makeDatasetFilename(std::size_t numberOfElems,
                                           std::size_t highestNumber,
                                           std::size_t sampleSize,
                                           std::size_t runs,
                                           std::string_view prefix = "dataset") {
        // “abbr” compatte per numeri grandi (1'000'000 → 1e6) – purely cosmetic
        auto fmt = [](std::size_t x) {
            if (x % 1'000'000'000 == 0) return std::format("{}e9", x / 1'000'000'000);
            if (x % 1'000'000 == 0) return std::format("{}e6", x / 1'000'000);
            if (x % 1'000 == 0) return std::format("{}e3", x / 1'000);
            return std::to_string(x);
        };

        return std::format("{}_N{}_H{}_S{}_R{}.bin",
                           prefix,
                           fmt(numberOfElems),
                           fmt(highestNumber),
                           fmt(sampleSize),
                           runs);
    }
} // namespace satp::io
