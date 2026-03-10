#include "satp/cli/detail/config/RunParameters.h"

#include <exception>
#include <limits>

#include "satp/hashing/HashFactory.h"
#include "satp/simulation/detail/merge/HeterogeneousMergeTypes.h"

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

        bool setLeftHashFunctionName(RunConfig &cfg, const string &value) {
            if (value == "default") {
                cfg.leftHashFunctionName = nullopt;
                return true;
            }
            try {
                const auto hash = hashing::getHashFunctionBy(value, 0u);
                cfg.leftHashFunctionName = string(hash->name());
            } catch (const exception &) {
                return false;
            }
            return true;
        }

        bool setRightHashFunctionName(RunConfig &cfg, const string &value) {
            if (value == "default") {
                cfg.rightHashFunctionName = nullopt;
                return true;
            }
            try {
                const auto hash = hashing::getHashFunctionBy(value, 0u);
                cfg.rightHashFunctionName = string(hash->name());
            } catch (const exception &) {
                return false;
            }
            return true;
        }

        bool setOptionalSeed(optional<uint32_t> &out, const string &value) {
            if (value == "dataset") {
                out = nullopt;
                return true;
            }
            uint32_t parsed = 0;
            if (!parseU32(value, parsed)) return false;
            out = parsed;
            return true;
        }

        bool setLeftHashSeed(RunConfig &cfg, const string &value) {
            return setOptionalSeed(cfg.leftHashSeed, value);
        }

        bool setRightHashSeed(RunConfig &cfg, const string &value) {
            return setOptionalSeed(cfg.rightHashSeed, value);
        }

        bool setK(RunConfig &cfg, const string &value) {
            return parseU32(value, cfg.k);
        }

        bool setOptionalK(optional<uint32_t> &out, const string &value) {
            if (value == "default") {
                out = nullopt;
                return true;
            }
            uint32_t parsed = 0;
            if (!parseU32(value, parsed)) return false;
            out = parsed;
            return true;
        }

        bool setLeftK(RunConfig &cfg, const string &value) {
            return setOptionalK(cfg.leftK, value);
        }

        bool setRightK(RunConfig &cfg, const string &value) {
            return setOptionalK(cfg.rightK, value);
        }

        bool setL(RunConfig &cfg, const string &value) {
            return parseU32(value, cfg.l);
        }

        bool setLLog(RunConfig &cfg, const string &value) {
            return parseU32(value, cfg.lLog);
        }

        bool setMergeStrategy(RunConfig &cfg, const string &value) {
            using satp::evaluation::MergeStrategy;
            if (value == "direct") {
                cfg.mergeStrategy = MergeStrategy::Direct;
                return true;
            }
            if (value == "reject") {
                cfg.mergeStrategy = MergeStrategy::Reject;
                return true;
            }
            if (value == "reduce_then_merge") {
                cfg.mergeStrategy = MergeStrategy::ReduceThenMerge;
                return true;
            }
            if (value == "unsafe_naive_merge") {
                cfg.mergeStrategy = MergeStrategy::UnsafeNaiveMerge;
                return true;
            }
            return false;
        }

        [[nodiscard]] const array<RunParamSpec, 13> &runParamSpecs() {
            static const array<RunParamSpec, 13> specs{{
                {"datasetPath", setDatasetPath},
                {"resultsNamespace", setResultsNamespace},
                {"hashFunction", setHashFunctionName},
                {"leftHashFunction", setLeftHashFunctionName},
                {"rightHashFunction", setRightHashFunctionName},
                {"leftHashSeed", setLeftHashSeed},
                {"rightHashSeed", setRightHashSeed},
                {"k", setK},
                {"leftK", setLeftK},
                {"rightK", setRightK},
                {"l", setL},
                {"lLog", setLLog},
                {"mergeStrategy", setMergeStrategy}
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

    const array<string_view, 13> &configurableParamNames() {
        static const array<string_view, 13> names{
            "datasetPath",
            "resultsNamespace",
            "hashFunction",
            "leftHashFunction",
            "rightHashFunction",
            "leftHashSeed",
            "rightHashSeed",
            "k",
            "leftK",
            "rightK",
            "l",
            "lLog",
            "mergeStrategy"
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
