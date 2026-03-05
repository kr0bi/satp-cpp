#include "satp/hashing/HashFactory.h"

#include <algorithm>
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

    const HashFunction &defaultHashFunction() {
        static const functions::SplitMix64 hash{};
        return hash;
    }

    const HashFunction &hashFunctionByName(const string_view name) {
        const string normalized = normalizeName(name);

        static const functions::SplitMix64 splitMix64{};
        static const functions::XXHash64 xxHash64{};
        static const functions::MurmurHash3 murmurHash3{};
        static const functions::SipHash24 sipHash24{};

        if (normalized == "splitmix64" || normalized == "splitmix") {
            return splitMix64;
        }
        if (normalized == "xxhash64" || normalized == "xxhash") {
            return xxHash64;
        }
        if (normalized == "murmurhash3" || normalized == "murmur3" || normalized == "murmur") {
            return murmurHash3;
        }
        if (normalized == "siphash24" || normalized == "siphash") {
            return sipHash24;
        }

        throw invalid_argument(
            "Unsupported hash function '" + string(name) +
            "'. Supported: splitmix64, xxhash64, murmurhash3, siphash24");
    }

    bool isSupportedHashFunction(const string_view name) {
        try {
            (void) hashFunctionByName(name);
            return true;
        } catch (const exception &) {
            return false;
        }
    }

    vector<string> hashFunctionNames() {
        return {
            "splitmix64",
            "xxhash64",
            "murmurhash3",
            "siphash24",
        };
    }
} // namespace satp::hashing

