#include "satp/algorithms/AlgorithmCatalog.h"

#include <cctype>
#include <stdexcept>

using namespace std;

namespace satp::algorithms::catalog {
    namespace {
        struct Entry {
            string_view id;
            string_view name;
        };

        [[nodiscard]] string normalize(string_view raw) {
            string out;
            out.reserve(raw.size());
            for (const char c : raw) {
                if (isalnum(static_cast<unsigned char>(c)) != 0) {
                    out.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));
                } else if (c == '+') {
                    out += "plus";
                }
            }
            return out;
        }

        [[nodiscard]] const array<Entry, 5> &allEntries() {
            static const array<Entry, 5> entries{{
            {"hllpp", "HyperLogLog++"},
            {"hll", "HyperLogLog"},
            {"ll", "LogLog"},
            {"pc", "Probabilistic Counting"},
            {"naive", "Naive"},
        }};
            return entries;
        }
    } // namespace

    const array<string_view, 4> &getIdsOfSupportedAlgorithms() {
        static const array<string_view, 4> ids{
            "hllpp",
            "hll",
            "ll",
            "pc",
        };
        return ids;
    }

    string getNameBy(const string_view id) {
        const string normalizedId = normalize(id);
        for (const auto &entry : allEntries()) {
            if (normalize(entry.id) == normalizedId) {
                return string(entry.name);
            }
        }
        throw invalid_argument("Unsupported algorithm id '" + string(id) + "'");
    }
} // namespace satp::algorithms::catalog
