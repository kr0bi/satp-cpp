#pragma once

#include <array>
#include <string>
#include <string_view>

#include "satp/cli/detail/CliTypes.h"

using namespace std;

namespace satp::cli::config {
    [[nodiscard]] bool setParam(RunConfig &cfg,
                                const string &param,
                                const string &value);

    [[nodiscard]] const array<string_view, 6> &configurableParamNames();

    [[nodiscard]] const array<string_view, 4> &supportedHashFunctionNames();
} // namespace satp::cli::config
