#pragma once

#include <vector>

#include "satp/simulation/ErrorAccumulator.h"
#include "satp/simulation/evaluationFramework/Context.h"
#include "satp/simulation/evaluationFramework/Detail.h"

using namespace std;

namespace satp::evaluation::modes::normal {
    template<typename Algo, typename... Args>
    Stats evaluate(const detail::EvaluationContext &context,
                   Args &&... ctorArgs) {
        if (detail::scopeIsEmpty(context.runs, context.sampleSize)) return {};

        auto bar = detail::makeProgressBar(context.runs * context.sampleSize);
        satp::io::BinaryDatasetPartitionReader reader(context.binaryDataset);

        vector<uint32_t> partitionValues;
        ErrorAccumulator accumulator;
        const double truth = static_cast<double>(context.distinctCount);

        for (size_t run = 0; run < context.runs; ++run) {
            reader.load(run, partitionValues);
            Algo algo = detail::makeAlgo<Algo>(context, forward<Args>(ctorArgs)...);
            detail::processValues(algo, partitionValues, bar);

            accumulator.add(static_cast<double>(algo.count()), truth);
        }

        detail::finishProgressBar(bar);
        return accumulator.toStats();
    }
} // namespace satp::evaluation::modes::normal
