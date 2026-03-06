#pragma once

#include <string>

#include "satp/cli/CliTypes.h"

using namespace std;

namespace satp::cli::config {
    [[nodiscard]] Command parseCommand(const string &line);
} // namespace satp::cli::config
