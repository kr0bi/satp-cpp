#include "NaiveCounting.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

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

    void NaiveCounting::merge(const Algorithm &other) {
        const auto *typed = dynamic_cast<const NaiveCounting *>(&other);
        if (typed == nullptr) {
            throw invalid_argument("NaiveCounting merge requires NaiveCounting");
        }
        merge(*typed);
    }

    void NaiveCounting::merge(const NaiveCounting &other) {
        ids.insert(other.ids.begin(), other.ids.end());
    }
} // namespace satp::algorithms
