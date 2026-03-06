#pragma once

#include <string>

#include "satp/simulation/EvaluationMetadata.h"

using namespace std;

namespace satp::evaluation {
    struct CsvRunDescriptor {
        string algorithmName;
        string algorithmParams;
        EvaluationMetadata metadata;
        double rseTheoretical = 0.0;
    };
} // namespace satp::evaluation
