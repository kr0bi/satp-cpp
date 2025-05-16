#include "NaiveCounting.h"

#include <algorithm>
#include <cstdint>

namespace satp::algorithms {
    void NaiveCounting::process(std::uint32_t id) {
        if (!ids.contains(id)) {
            ids.insert(id);
        }
    }

    std::uint64_t NaiveCounting::count() {
        return ids.size();
    }

    void NaiveCounting::reset() {
        ids.clear();
    }

    std::string NaiveCounting::getName() {
        return "Naive";
    }
} // namespace satp::algorithms
