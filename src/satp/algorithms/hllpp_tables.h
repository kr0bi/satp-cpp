#pragma once
#include <array>
#include <cstddef>

namespace satp::algorithms::hllpp_tables {
    inline constexpr std::size_t MIN_K = 4;
    inline constexpr std::size_t MAX_K = 18;

    extern const std::array<std::pair<double, double>, 97> &table_for_k(std::size_t k);
} // namespace satp::algorithms::hllpp_tables
