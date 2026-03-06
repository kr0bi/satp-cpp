#include "satp/cli/detail/config/RunParameters.h"

#include <exception>
#include <limits>

#include "satp/hashing/HashFactory.h"

using namespace std;

namespace satp::cli::config {
    namespace {
        using ParamSetter = bool (*)(RunConfig &, const string &);

        struct RunParamSpec {
            string_view key;
            ParamSetter set;
        };

        bool parseU32(const string &raw, uint32_t &out) {
            try {
                size_t idx = 0;
                const unsigned long value = stoul(raw, &idx);
                if (idx != raw.size() || value > numeric_limits<uint32_t>::max()) {
                    return false;
                }
                out = static_cast<uint32_t>(value);
                return true;
            } catch (const exception &) {
                return false;
            }
        }

        bool setDatasetPath(RunConfig &cfg, const string &value) {
            cfg.datasetPath = value;
            return true;
        }

        bool setResultsNamespace(RunConfig &cfg, const string &value) {
            cfg.resultsNamespace = value;
            return true;
        }

        bool setHashFunctionName(RunConfig &cfg, const string &value) {
            try {
                // CLI parsing does not know dataset seed yet: use a dummy seed only
                // to validate/normalize the hash function name.
                const auto hash = hashing::getHashFunctionBy(value, 0u);
                cfg.hashFunctionName = hash->name();
            } catch (const exception &) {
                return false;
            }
            return true;
        }

        bool setK(RunConfig &cfg, const string &value) {
            return parseU32(value, cfg.k);
        }

        bool setL(RunConfig &cfg, const string &value) {
            return parseU32(value, cfg.l);
        }

        bool setLLog(RunConfig &cfg, const string &value) {
            return parseU32(value, cfg.lLog);
        }

        [[nodiscard]] const array<RunParamSpec, 6> &runParamSpecs() {
            static const array<RunParamSpec, 6> specs{{
                {"datasetPath", setDatasetPath},
                {"resultsNamespace", setResultsNamespace},
                {"hashFunction", setHashFunctionName},
                {"k", setK},
                {"l", setL},
                {"lLog", setLLog}
            }};
            return specs;
        }
    } // namespace

    bool setParam(RunConfig &cfg,
                  const string &param,
                  const string &value) {
        for (const auto &spec : runParamSpecs()) {
            if (param == spec.key) {
                return spec.set(cfg, value);
            }
        }
        return false;
    }

    const array<string_view, 6> &configurableParamNames() {
        static const array<string_view, 6> names{
            "datasetPath",
            "resultsNamespace",
            "hashFunction",
            "k",
            "l",
            "lLog"
        };
        return names;
    }

    const array<string_view, 4> &supportedHashFunctionNames() {
        static const array<string_view, 4> names{
            "splitmix64",
            "xxhash64",
            "murmurhash3",
            "siphash24"
        };
        return names;
    }
} // namespace satp::cli::config
