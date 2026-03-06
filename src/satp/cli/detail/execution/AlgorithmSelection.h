#pragma once

#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

namespace satp::cli::executor {
    using SelectedAlgorithms = unordered_set<string>;

    [[nodiscard]] SelectedAlgorithms collectRequestedAlgorithms(
        const vector<string> &algs);

    [[nodiscard]] bool shouldRun(const SelectedAlgorithms &selected,
                                 const string &algorithmId);
} // namespace satp::cli::executor
