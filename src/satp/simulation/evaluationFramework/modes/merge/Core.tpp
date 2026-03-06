#pragma once

#include <cmath>
#include <vector>

#include "satp/simulation/EvaluationMetadata.h"
#include "satp/simulation/evaluationFramework/AlgorithmFactory.h"
#include "satp/simulation/evaluationFramework/Context.h"
#include "satp/simulation/evaluationFramework/DatasetHelpers.h"

using namespace std;

namespace satp::evaluation::modes::merge {
    template<typename Algo, typename... Args>
    vector<MergePairPoint> evaluate(const detail::EvaluationContext &context,
                                    Args &&... ctorArgs) {
        static_assert(detail::MergeableAlgorithm<Algo>,
                      "merge::evaluate requires Algo::merge(const Algo&)");

        if (context.metadata.runs < 2u || hasEmptyDataset(context.metadata)) return {};

        const size_t pairCount = context.metadata.runs / 2u;
        detail::startProgress(context.progress, pairCount * context.metadata.sampleSize * 4u);
        satp::io::BinaryDatasetPartitionReader reader(context.binaryDataset);

        vector<uint32_t> partA;
        vector<uint32_t> partB;
        vector<MergePairPoint> points;
        points.reserve(pairCount);

        for (size_t pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
            const size_t idxA = 2u * pairIndex;
            const size_t idxB = idxA + 1u;
            reader.load(idxA, partA);
            reader.load(idxB, partB);

            Algo sketchA = detail::makeAlgo<Algo>(context, std::forward<Args>(ctorArgs)...);
            detail::processValues(sketchA, partA, context.progress);

            Algo sketchB = detail::makeAlgo<Algo>(context, std::forward<Args>(ctorArgs)...);
            detail::processValues(sketchB, partB, context.progress);

            Algo merged = sketchA;
            merged.merge(sketchB);

            Algo serial = detail::makeAlgo<Algo>(context, std::forward<Args>(ctorArgs)...);
            detail::processValues(serial, partA, context.progress);
            detail::processValues(serial, partB, context.progress);

            const double estimateMerge = static_cast<double>(merged.count());
            const double estimateSerial = static_cast<double>(serial.count());
            const double deltaAbs = abs(estimateMerge - estimateSerial);
            const double deltaRel = (estimateSerial != 0.0) ? (deltaAbs / estimateSerial) : 0.0;

            points.push_back({
                pairIndex,
                estimateMerge,
                estimateSerial,
                deltaAbs,
                deltaRel
            });
        }

        detail::finishProgress(context.progress);
        return points;
    }
} // namespace satp::evaluation::modes::merge
