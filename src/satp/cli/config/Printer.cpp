#include "satp/cli/config/Printer.h"

#include <iostream>

#include "satp/algorithms/AlgorithmCatalog.h"
#include "satp/cli/config/RunConfigParams.h"
#include "satp/cli/config/RuntimeContext.h"

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
} // namespace satp::cli::config
