#include "NaiveCounting.h"

#include <algorithm>
#include <cstdint>

namespace satp::algorithms {

    void NaiveCounting::process(std::uint32_t id)
    {
        // equivalente a ArrayList.contains() + add():
        if (std::ranges::find(ids_, id) == ids_.end()) {
            ids_.push_back(id);
        }
    }

    std::uint64_t NaiveCounting::count() const
    {
        return ids_.size();
    }

    void NaiveCounting::reset()
    {
        ids_.clear();
    }

} // namespace satp::algorithms
