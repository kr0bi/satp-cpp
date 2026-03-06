#pragma once

#include <utility>

#include "satp/simulation/CsvResultWriter.h"
#include "satp/simulation/evaluationFramework/Context.h"
#include "satp/simulation/evaluationFramework/modes/streaming/Core.tpp"

using namespace std;

namespace satp::evaluation::modes::streaming {
    template<typename Algo, typename... Args>
    vector<StreamingPointStats> evaluateToCsv(
        const detail::EvaluationContext &context,
        const filesystem::path &csvPath,
        const string &algorithmParams,
        const double rseTheoretical,
        Args &&... ctorArgs) {
        Algo algo = detail::makeAlgo<Algo>(context, ctorArgs...);
        auto series = evaluate<Algo>(context, forward<Args>(ctorArgs)...);
        CsvResultWriter::appendStreaming(
            csvPath,
            algo.getName(),
            algorithmParams,
            context.runs,
            context.sampleSize,
            context.distinctCount,
            context.seed,
            rseTheoretical,
            series);
        return series;
    }
} // namespace satp::evaluation::modes::streaming
