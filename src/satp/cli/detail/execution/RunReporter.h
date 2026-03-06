#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "satp/cli/detail/CliTypes.h"
#include "satp/simulation/Simulation.h"

using namespace std;

namespace satp::cli::executor {
    [[nodiscard]] const char *modeLabel(RunMode mode);

    [[nodiscard]] double rseHll(uint32_t k);
    [[nodiscard]] double rseLogLog(uint32_t k);
    [[nodiscard]] double rseUnknown();

    [[nodiscard]] string algorithmLogPrefix(const AlgorithmRunSpec &spec);

    void printRunContext(const DatasetRuntimeContext &ctx,
                         RunMode mode,
                         const string &hashName);

    void printStreamingSummary(const AlgorithmRunSpec &spec,
                               const filesystem::path &csvPath,
                               const satp::evaluation::StreamingPointStats &lastPoint);

    void printMergeSummary(const AlgorithmRunSpec &spec,
                           const filesystem::path &csvPath,
                           const satp::evaluation::MergePairStats &stats);
} // namespace satp::cli::executor
