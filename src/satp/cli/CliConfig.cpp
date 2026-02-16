#include "satp/cli/CliConfig.h"

#include <iostream>
#include <limits>
#include <sstream>

#include "satp/cli/PathUtils.h"
#include "satp/io/BinaryDatasetIO.h"

namespace satp::cli::config {
    namespace {
        bool parseU32(const std::string &raw, std::uint32_t &out) {
            try {
                std::size_t idx = 0;
                const unsigned long value = std::stoul(raw, &idx);
                if (idx != raw.size() || value > std::numeric_limits<std::uint32_t>::max()) {
                    return false;
                }
                out = static_cast<std::uint32_t>(value);
                return true;
            } catch (const std::exception &) {
                return false;
            }
        }
    } // namespace

    Command parseCommand(const std::string &line) {
        std::istringstream iss(line);
        Command cmd;
        iss >> cmd.name;
        for (std::string token; iss >> token;) {
            cmd.args.push_back(token);
        }
        return cmd;
    }

    bool setParam(RunConfig &cfg,
                  const std::string &param,
                  const std::string &value) {
        if (param == "datasetPath") {
            cfg.datasetPath = value;
            return true;
        }
        if (param == "k") return parseU32(value, cfg.k);
        if (param == "l") return parseU32(value, cfg.l);
        if (param == "lLog") return parseU32(value, cfg.lLog);
        return false;
    }

    void printHelp() {
        std::cout
            << "Comandi disponibili:\n"
            << "  help                         Mostra questo help\n"
            << "  show                         Stampa i parametri correnti\n"
            << "  list                         Elenca algoritmi supportati\n"
            << "  set <param> <value>          Imposta un parametro\n"
            << "  run <algo|all>               Esegue uno o piu' algoritmi (modalita' normale)\n"
            << "  runstream <algo|all>         Esegue uno o piu' algoritmi (modalita' streaming)\n"
            << "  runmerge <algo|all>          Esegue benchmark merge a coppie (0-1,2-3,...)\n"
            << "                               CSV automatico in results/<algoritmo>/<params>/\n"
            << "  quit                         Esce\n";
    }

    void printAlgorithms() {
        std::cout
            << "Algoritmi:\n"
            << "  hllpp  (HyperLogLog++)\n"
            << "  hll    (HyperLogLog)\n"
            << "  ll     (LogLog)\n"
            << "  pc     (ProbabilisticCounting)\n";
    }

    std::optional<DatasetView> readDatasetView(const std::string &datasetPath) {
        try {
            const auto index = satp::io::indexBinaryDataset(datasetPath);
            return DatasetView{
                index.info.elements_per_partition,
                index.info.partition_count,
                index.info.seed
            };
        } catch (const std::exception &) {
            return std::nullopt;
        }
    }

    void printConfig(const RunConfig &cfg) {
        DatasetView view;
        if (const auto loaded = readDatasetView(cfg.datasetPath)) {
            view = *loaded;
        }

        std::cout
            << "Parametri correnti:\n"
            << "  datasetPath   = " << cfg.datasetPath << '\n'
            << "  sampleSize    = " << view.sampleSize << " (dal dataset)\n"
            << "  runs          = " << view.runs << " (dal dataset)\n"
            << "  seed          = " << view.seed << " (dal dataset)\n"
            << "  k             = " << cfg.k << '\n'
            << "  l             = " << cfg.l << '\n'
            << "  lLog          = " << cfg.lLog << '\n';
    }

    DatasetRuntimeContext loadDatasetRuntimeContext(const RunConfig &cfg) {
        DatasetRuntimeContext ctx;
        ctx.index = satp::io::indexBinaryDataset(cfg.datasetPath);
        ctx.sampleSize = ctx.index.info.elements_per_partition;
        ctx.runs = ctx.index.info.partition_count;
        ctx.seed = ctx.index.info.seed;
        ctx.repoRoot = path_utils::detectRepoRoot(cfg.datasetPath);
        return ctx;
    }
} // namespace satp::cli::config
