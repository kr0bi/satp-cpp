#pragma once

// This module coordinates sketching experiments on binary datasets. It exposes
// the evaluation framework, progress callbacks, streaming checkpoint planning,
// experiment statistics, merge summaries, and CSV result writing.

#include "satp/simulation/detail/framework/EvaluationFramework.h"
#include "satp/simulation/detail/framework/EvaluationMetadata.h"
#include "satp/simulation/detail/framework/ProgressCallbacks.h"
#include "satp/simulation/detail/merge/MergeSummary.h"
#include "satp/simulation/detail/metrics/Statistics.h"
#include "satp/simulation/detail/results/CsvResultWriter.h"
#include "satp/simulation/detail/streaming/CheckpointPlanner.h"
