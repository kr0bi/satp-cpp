#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "satp/hashing/HashFunction.h"
#include "satp/io/BinaryDatasetIO.h"

using namespace std;

namespace satp::evaluation::detail {
    struct EvaluationContext {
        const io::BinaryDatasetIndex &binaryDataset;
        size_t runs = 0;
        size_t sampleSize = 0;
        size_t distinctCount = 0;
        uint32_t seed = 0;
        const hashing::HashFunction &hashFunction;
    };

    [[nodiscard]] inline bool scopeIsEmpty(
        const size_t runs,
        const size_t sampleSize) noexcept {
        return runs == 0u || sampleSize == 0u;
    }

    template<typename Algo, typename... Args>
    Algo makeAlgo(const EvaluationContext &context, Args &&... ctorArgs) {
        static_assert(constructible_from<Algo, Args..., const hashing::HashFunction &>,
                      "Algorithm must be constructible with (..., const hashing::HashFunction&)");
        return Algo(forward<Args>(ctorArgs)..., context.hashFunction);
    }
} // namespace satp::evaluation::detail
