#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "satp/hashing/HashFunction.h"

using namespace std;

namespace satp::hashing {
    [[nodiscard]] const HashFunction &defaultHashFunction();

    [[nodiscard]] const HashFunction &hashFunctionByName(string_view name);

    [[nodiscard]] bool isSupportedHashFunction(string_view name);

    [[nodiscard]] vector<string> hashFunctionNames();
} // namespace satp::hashing

