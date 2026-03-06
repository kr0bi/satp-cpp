#pragma once

#include <concepts>
#include <utility>

#include "satp/simulation/detail/framework/EvaluationContext.h"

using namespace std;

namespace satp::evaluation::detail {
    template<typename Algo>
    concept MergeableAlgorithm = requires(Algo a, const Algo &b) {
        { a.merge(b) } -> same_as<void>;
    };

    template<typename Algo, typename... Args>
    Algo makeAlgo(const EvaluationContext &context, Args &&... ctorArgs) {
        static_assert(constructible_from<Algo, Args..., const hashing::HashFunction &>,
                      "Algorithm must be constructible with (..., const hashing::HashFunction&)");
        return Algo(std::forward<Args>(ctorArgs)..., context.hashFunction);
    }
} // namespace satp::evaluation::detail
