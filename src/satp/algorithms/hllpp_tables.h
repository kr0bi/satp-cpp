#pragma once
#include <cstdint>
#include <cstddef>
#include <utility>
#include <vector>

using namespace std;

namespace satp::algorithms::hllpp_tables {
    inline constexpr size_t MIN_K = 4;
    inline constexpr size_t MAX_K = 18;

    extern const vector<pair<double, double> > &table_for_k(size_t k);

    extern uint32_t threshold_for_k(size_t k);
} // namespace satp::algorithms::hllpp_tables
