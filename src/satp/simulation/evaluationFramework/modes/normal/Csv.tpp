#pragma once

#include <utility>

#include "satp/simulation/CsvResultWriter.h"
#include "satp/simulation/evaluationFramework/Context.h"
#include "satp/simulation/evaluationFramework/modes/normal/Core.tpp"

using namespace std;

namespace satp::evaluation::modes::normal {
    template<typename Algo, typename... Args>
    Stats evaluateToCsv(const detail::EvaluationContext &context,
                        const filesystem::path &csvPath,
                        const string &algorithmParams,
                        const double rseTheoretical,
                        Args &&... ctorArgs) {
        Algo algo = detail::makeAlgo<Algo>(context, ctorArgs...);
        const Stats stats = evaluate<Algo>(context, forward<Args>(ctorArgs)...);
        CsvResultWriter::appendNormal(
            csvPath,
            algo.getName(),
            algorithmParams,
            context.runs,
            context.sampleSize,
            context.distinctCount,
            context.seed,
            rseTheoretical,
            stats);
        return stats;
    }
} // namespace satp::evaluation::modes::normal
