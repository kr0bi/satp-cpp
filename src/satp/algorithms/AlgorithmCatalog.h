#pragma once

#include <array>
#include <string>
#include <string_view>

using namespace std;

namespace satp::algorithms::catalog {
    [[nodiscard]] string getNameBy(string_view id);

    [[nodiscard]] const array<string_view, 4> &getIdsOfSupportedAlgorithms();
} // namespace satp::algorithms::catalog
