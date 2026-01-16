#pragma once
#include <cstddef>
#include <utility>
#include <vector>

namespace satp::algorithms::hllpp_tables {
    inline constexpr std::size_t MIN_K = 4;
    inline constexpr std::size_t MAX_K = 18;

    extern const std::vector<std::pair<double, double> > &table_for_k(std::size_t k);
} // namespace satp::algorithms::hllpp_tables
