#include "satp/cli/detail/config/ConfigPrinter.h"

#include <iostream>

#include "satp/algorithms/AlgorithmCatalog.h"
#include "satp/cli/detail/config/DatasetRuntime.h"
#include "satp/cli/detail/config/RunParameters.h"
#include "satp/simulation/detail/merge/HeterogeneousMergeTypes.h"

using namespace std;

namespace satp::cli::config {
    namespace {
        [[nodiscard]] const string &runParamListForHelp() {
            static const string value = [] {
                string out;
                for (const auto name : configurableParamNames()) {
                    if (!out.empty()) out += ", ";
                    out += name;
                }
                return out;
            }();
            return value;
        }
    } // namespace

    void printHelp() {
        cout
            << "Comandi disponibili:\n"
            << "  help                         Mostra questo help\n"
            << "  show                         Stampa i parametri correnti\n"
            << "  list                         Elenca algoritmi supportati\n"
            << "  set <param> <value>          Imposta un parametro\n"
            << "                               Parametri: " << runParamListForHelp() << '\n'
            << "  runstream <algo|all>         Esegue uno o piu' algoritmi (modalita' streaming)\n"
            << "  runmerge <algo|all>          Esegue benchmark merge a coppie (0-1,2-3,...)\n"
            << "  runmergehet <algo|all>       Esegue benchmark di merge eterogeneo (attualmente: hllpp)\n"
            << "                               CSV automatico in results/<namespace>/<mode>/<algoritmo>/<hash>/<params>/\n"
            << "  quit                         Esce\n";
    }

    void printAlgorithms() {
        cout << "Algoritmi:\n";
        for (const auto id : satp::algorithms::catalog::getIdsOfSupportedAlgorithms()) {
            cout << "  " << id << "  (" << satp::algorithms::catalog::getNameBy(id) << ")\n";
        }

        cout << "Hash functions:\n";
        for (const auto name : supportedHashFunctionNames()) {
            cout << "  " << name << '\n';
        }
    }

    void printConfig(const RunConfig &cfg) {
        DatasetView view;
        bool hasDatasetView = false;
        if (const auto loaded = readDatasetView(cfg.datasetPath)) {
            view = *loaded;
            hasDatasetView = true;
        }

        const string leftHashName = cfg.leftHashFunctionName.value_or(cfg.hashFunctionName);
        const string rightHashName = cfg.rightHashFunctionName.value_or(cfg.hashFunctionName);
        const uint32_t leftK = cfg.leftK.value_or(cfg.k);
        const uint32_t rightK = cfg.rightK.value_or(cfg.k);

        auto describeHashSeed = [&](const optional<uint32_t> &seed) {
            if (seed.has_value()) return to_string(*seed);
            if (hasDatasetView) return to_string(view.seed) + " (dataset)";
            return string("<dataset seed>");
        };

        cout
            << "Parametri correnti:\n"
            << "  datasetPath   = " << cfg.datasetPath << '\n'
            << "  resultsNs     = " << cfg.resultsNamespace << '\n'
            << "  hashFunction  = " << cfg.hashFunctionName << '\n'
            << "  leftHashFn    = " << leftHashName
            << (cfg.leftHashFunctionName.has_value() ? "" : " (fallback hashFunction)") << '\n'
            << "  rightHashFn   = " << rightHashName
            << (cfg.rightHashFunctionName.has_value() ? "" : " (fallback hashFunction)") << '\n'
            << "  sampleSize    = " << view.sampleSize << " (dal dataset)\n"
            << "  runs          = " << view.runs << " (dal dataset)\n"
            << "  seed          = " << view.seed << " (dal dataset)\n"
            << "  k             = " << cfg.k << '\n'
            << "  leftK         = " << leftK
            << (cfg.leftK.has_value() ? "" : " (fallback k)") << '\n'
            << "  rightK        = " << rightK
            << (cfg.rightK.has_value() ? "" : " (fallback k)") << '\n'
            << "  leftHashSeed  = " << describeHashSeed(cfg.leftHashSeed) << '\n'
            << "  rightHashSeed = " << describeHashSeed(cfg.rightHashSeed) << '\n'
            << "  l             = " << cfg.l << '\n'
            << "  lLog          = " << cfg.lLog << '\n'
            << "  mergeStrategy = " << satp::evaluation::toString(cfg.mergeStrategy) << '\n';
    }
} // namespace satp::cli::config
