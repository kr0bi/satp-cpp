#include "satp/cli/executor/Selection.h"

using namespace std;

namespace satp::cli::executor {
    SelectedAlgorithms collectRequestedAlgorithms(const vector<string> &algs) {
        SelectedAlgorithms selected;
        selected.reserve(algs.size());
        for (const auto &name : algs) {
            selected.insert(name);
        }
        return selected;
    }

    bool shouldRun(const SelectedAlgorithms &selected,
                   const string &algorithmId) {
        return selected.find("all") != selected.end() ||
               selected.find(algorithmId) != selected.end();
    }
} // namespace satp::cli::executor
