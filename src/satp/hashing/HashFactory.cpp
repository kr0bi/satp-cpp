#include "satp/hashing/HashFactory.h"

#include <cctype>
#include <stdexcept>

#include "satp/hashing/functions/MurmurHash3.h"
#include "satp/hashing/functions/SipHash24.h"
#include "satp/hashing/functions/SplitMix64.h"
#include "satp/hashing/functions/XXHash64.h"

using namespace std;

namespace satp::hashing {
    namespace {
        [[nodiscard]] string normalizeName(const string_view raw) {
            string out;
            out.reserve(raw.size());
            for (const char c : raw) {
                if (isalnum(static_cast<unsigned char>(c)) != 0) {
                    out.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));
                }
            }
            return out;
        }
    } // namespace

    unique_ptr<HashFunction> getHashFunctionBy(
        const optional<string_view> name,
        const optional<uint32_t> seed) {
        if (!name.has_value()) {
            if (seed.has_value()) {
                throw invalid_argument("Hash seed provided without hash function name.");
            }
            return make_unique<functions::SplitMix64>();
        }

        const string normalized = normalizeName(*name);
        if (normalized.empty()) {
            return make_unique<functions::SplitMix64>();
        }

        if (!seed.has_value()) {
            throw invalid_argument("Hash seed is required when hash function name is provided.");
        }

        if (normalized == "splitmix64" || normalized == "splitmix") {
            return make_unique<functions::SplitMix64>();
        }
        if (normalized == "xxhash64" || normalized == "xxhash") {
            return make_unique<functions::XXHash64>(static_cast<uint64_t>(*seed));
        }
        if (normalized == "murmurhash3" || normalized == "murmur3" || normalized == "murmur") {
            return make_unique<functions::MurmurHash3>(*seed);
        }
        if (normalized == "siphash24" || normalized == "siphash") {
            return make_unique<functions::SipHash24>();
        }

        throw invalid_argument(
            "Unsupported hash function '" + string(*name) +
            "'. Supported: splitmix64, xxhash64, murmurhash3, siphash24");
    }
} // namespace satp::hashing
