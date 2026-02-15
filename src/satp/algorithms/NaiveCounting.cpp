#include "NaiveCounting.h"

#include <algorithm>
#include <cstdint>

namespace satp::algorithms {
    void NaiveCounting::process(uint32_t id) {
        if (!ids.contains(id)) {
            ids.insert(id);
        }
    }

    uint64_t NaiveCounting::count() {
        return ids.size();
    }

    void NaiveCounting::reset() {
        ids.clear();
    }

    string NaiveCounting::getName() {
        return "Naive";
    }
} // namespace satp::algorithms
