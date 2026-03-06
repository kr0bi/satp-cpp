#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "satp/simulation/detail/framework/ProgressCallbacks.h"

using namespace std;

namespace satp::evaluation::detail {
    inline void startProgress(const ProgressCallbacks *progress, const size_t totalTicks) {
        if (progress != nullptr && progress->onStart) {
            progress->onStart(totalTicks);
        }
    }

    inline void advanceProgress(const ProgressCallbacks *progress, const size_t ticks = 1u) {
        if (progress != nullptr && progress->onAdvance) {
            progress->onAdvance(ticks);
        }
    }

    inline void finishProgress(const ProgressCallbacks *progress) {
        if (progress != nullptr && progress->onFinish) {
            progress->onFinish();
        }
    }

    template<typename Algo>
    inline void processValues(Algo &algo,
                              const vector<uint32_t> &values,
                              const ProgressCallbacks *progress) {
        for (const auto value : values) {
            algo.process(value);
            advanceProgress(progress);
        }
    }

    inline void validateStreamingPartition(const vector<uint32_t> &values,
                                           const vector<uint8_t> &truthBits,
                                           const size_t sampleSize) {
        if (values.size() != sampleSize) {
            throw runtime_error("Invalid binary dataset: partition size mismatch while streaming");
        }
        if (truthBits.size() != (sampleSize + 7u) / 8u) {
            throw runtime_error("Invalid binary dataset: truth bitset size mismatch while streaming");
        }
    }

    [[nodiscard]] inline bool truthBitIsSet(const vector<uint8_t> &truthBits,
                                            const size_t index) noexcept {
        const uint8_t byte = truthBits[index >> 3u];
        return ((byte >> (index & 7u)) & 0x1u) != 0;
    }
} // namespace satp::evaluation::detail
