#pragma once

#include <utility>

#include "satp/simulation/CsvResultWriter.h"
#include "satp/simulation/evaluationFramework/Context.h"
#include "satp/simulation/evaluationFramework/Detail.h"
#include "satp/simulation/evaluationFramework/modes/merge/Core.tpp"

using namespace std;

namespace satp::evaluation::modes::merge {
    template<typename Algo, typename... Args>
    MergePairStats evaluateToCsv(
        const detail::EvaluationContext &context,
        const filesystem::path &csvPath,
        const string &algorithmParams,
        Args &&... ctorArgs) {
        Algo algo = detail::makeAlgo<Algo>(context, ctorArgs...);
        const auto points = evaluate<Algo>(context, forward<Args>(ctorArgs)...);
        CsvResultWriter::appendMergePairs(
            csvPath,
            algo.getName(),
            algorithmParams,
            points.size(),
            context.sampleSize,
            context.seed,
            points);
        return detail::summarizeMergePairs(points);
    }
} // namespace satp::evaluation::modes::merge
