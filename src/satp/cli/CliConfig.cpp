#include "satp/cli/CliConfig.h"

#include <array>
#include <iostream>
#include <limits>
#include <sstream>

#include "satp/cli/PathUtils.h"
#include "satp/hashing/HashFactory.h"
#include "satp/io/BinaryDatasetIO.h"

using namespace std;

namespace satp::cli::config {
    namespace {
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

        [[nodiscard]] const array<const char *, 4> &supportedHashFunctionNames() {
            static const array<const char *, 4> names{
                "splitmix64",
                "xxhash64",
                "murmurhash3",
                "siphash24"
            };
            return names;
        }
    } // namespace

    Command parseCommand(const string &line) {
        istringstream iss(line);
        Command cmd;
        iss >> cmd.name;
        for (string token; iss >> token;) {
            cmd.args.push_back(token);
        }
        return cmd;
    }

    bool setParam(RunConfig &cfg,
                  const string &param,
                  const string &value) {
        if (param == "datasetPath") {
            cfg.datasetPath = value;
            return true;
        }
        if (param == "resultsNamespace") {
            cfg.resultsNamespace = value;
            return true;
        }
        if (param == "hashFunction") {
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
        if (param == "k") return parseU32(value, cfg.k);
        if (param == "l") return parseU32(value, cfg.l);
        if (param == "lLog") return parseU32(value, cfg.lLog);
        return false;
    }

    void printHelp() {
        cout
            << "Comandi disponibili:\n"
            << "  help                         Mostra questo help\n"
            << "  show                         Stampa i parametri correnti\n"
            << "  list                         Elenca algoritmi supportati\n"
            << "  set <param> <value>          Imposta un parametro\n"
            << "                               Parametri: datasetPath, resultsNamespace, hashFunction, k, l, lLog\n"
            << "  run <algo|all>               Esegue uno o piu' algoritmi (modalita' normale)\n"
            << "  runstream <algo|all>         Esegue uno o piu' algoritmi (modalita' streaming)\n"
            << "  runmerge <algo|all>          Esegue benchmark merge a coppie (0-1,2-3,...)\n"
            << "                               CSV automatico in results/<namespace>/<algoritmo>/<params>/\n"
            << "  quit                         Esce\n";
    }

    void printAlgorithms() {
        cout
            << "Algoritmi:\n"
            << "  hllpp  (HyperLogLog++)\n"
            << "  hll    (HyperLogLog)\n"
            << "  ll     (LogLog)\n"
            << "  pc     (ProbabilisticCounting)\n"
            << "Hash functions:\n";
        for (const auto *name : supportedHashFunctionNames()) {
            cout << "  " << name << '\n';
        }
    }

    optional<DatasetView> readDatasetView(const string &datasetPath) {
        try {
            const auto index = satp::io::indexBinaryDataset(datasetPath);
            return DatasetView{
                index.info.elements_per_partition,
                index.info.partition_count,
                index.info.seed
            };
        } catch (const exception &) {
            return nullopt;
        }
    }

    void printConfig(const RunConfig &cfg) {
        DatasetView view;
        if (const auto loaded = readDatasetView(cfg.datasetPath)) {
            view = *loaded;
        }

        cout
            << "Parametri correnti:\n"
            << "  datasetPath   = " << cfg.datasetPath << '\n'
            << "  resultsNs     = " << cfg.resultsNamespace << '\n'
            << "  hashFunction  = " << cfg.hashFunctionName << '\n'
            << "  sampleSize    = " << view.sampleSize << " (dal dataset)\n"
            << "  runs          = " << view.runs << " (dal dataset)\n"
            << "  seed          = " << view.seed << " (dal dataset)\n"
            << "  k             = " << cfg.k << '\n'
            << "  l             = " << cfg.l << '\n'
            << "  lLog          = " << cfg.lLog << '\n';
    }

    DatasetRuntimeContext loadDatasetRuntimeContext(const RunConfig &cfg) {
        DatasetRuntimeContext ctx;
        ctx.index = io::indexBinaryDataset(cfg.datasetPath);
        ctx.sampleSize = ctx.index.info.elements_per_partition;
        ctx.runs = ctx.index.info.partition_count;
        ctx.seed = ctx.index.info.seed;
        ctx.resultsNamespace = cfg.resultsNamespace;
        ctx.repoRoot = path_utils::detectRepoRoot(cfg.datasetPath);
        return ctx;
    }
} // namespace satp::cli::config
