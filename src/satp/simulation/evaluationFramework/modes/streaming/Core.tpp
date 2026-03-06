#pragma once

#include <cstdint>
#include <vector>

#include "satp/simulation/ErrorAccumulator.h"
#include "satp/simulation/StreamingCheckpointBuilder.h"
#include "satp/simulation/evaluationFramework/Context.h"
#include "satp/simulation/evaluationFramework/Detail.h"

using namespace std;

namespace satp::evaluation::modes::streaming {
    template<typename Algo, typename... Args>
    vector<StreamingPointStats> evaluate(const detail::EvaluationContext &context,
                                         Args &&... ctorArgs) {
        if (detail::scopeIsEmpty(context.runs, context.sampleSize)) return {};

        const auto checkpointPositions = StreamingCheckpointBuilder::build(
            context.sampleSize,
            EvaluationFramework::DEFAULT_STREAMING_CHECKPOINTS);

        auto bar = detail::makeProgressBar(context.runs * context.sampleSize);
        satp::io::BinaryDatasetPartitionReader reader(context.binaryDataset);

        vector<ErrorAccumulator> accumulators(checkpointPositions.size());
        vector<uint32_t> partitionValues;
        vector<uint8_t> partitionTruthBits;

        for (size_t run = 0; run < context.runs; ++run) {
            reader.loadWithTruthBits(run, partitionValues, partitionTruthBits);
            detail::validateStreamingPartition(partitionValues, partitionTruthBits, context.sampleSize);

            Algo algo = detail::makeAlgo<Algo>(context, forward<Args>(ctorArgs)...);
            uint64_t truthPrefix = 0;
            size_t checkpointIndex = 0;

            for (size_t t = 0; t < context.sampleSize; ++t) {
                algo.process(partitionValues[t]);

                const bool isNew = detail::truthBitIsSet(partitionTruthBits, t);
                if (isNew) {
                    ++truthPrefix;
                }

                const size_t elementIndex = t + 1u;
                if (checkpointIndex < checkpointPositions.size()
                    && elementIndex == checkpointPositions[checkpointIndex]) {
                    accumulators[checkpointIndex].add(
                        static_cast<double>(algo.count()),
                        static_cast<double>(truthPrefix));
                    ++checkpointIndex;
                }

                bar.tick();
            }
        }

        detail::finishProgressBar(bar);

        vector<StreamingPointStats> out;
        out.reserve(checkpointPositions.size());
        for (size_t i = 0; i < checkpointPositions.size(); ++i) {
            out.push_back(accumulators[i].toStreamingPoint(checkpointPositions[i]));
        }
        return out;
    }
} // namespace satp::evaluation::modes::streaming
