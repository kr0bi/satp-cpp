#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "satp/hashing/HashFunction.h"

using namespace std;

namespace satp::hashing {
    [[nodiscard]] unique_ptr<HashFunction> getHashFunctionBy(
        optional<string_view> name = nullopt,
        optional<uint32_t> seed = nullopt);
} // namespace satp::hashing
