#pragma once

// This module indexes and loads the binary datasets used to run sketching
// experiments. It is the single public entrypoint for dataset metadata,
// partition reads, and truth-bit access.

#include "satp/dataset/detail/DatasetAccess.h"
#include "satp/dataset/detail/DatasetTypes.h"
