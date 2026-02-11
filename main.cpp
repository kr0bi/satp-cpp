#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_set>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "src/satp/algorithms/HyperLogLog.h"
#include "src/satp/algorithms/HyperLogLogPlusPlus.h"
#include "src/satp/algorithms/LogLog.h"
#include "src/satp/algorithms/ProbabilisticCounting.h"
#include "src/satp/io/BinaryDatasetIO.h"
#include "src/satp/simulation/EvaluationFramework.h"

namespace eval = satp::evaluation;
namespace alg = satp::algorithms;
struct RunConfig {
    std::string datasetPath = "compressed_dataset.bin";

    std::uint32_t k = 16; // registri per HLL/LL
    std::uint32_t l = 16; // bitmap PC
    std::uint32_t lLog = 32; // bitmap LL

    std::string csvPath = "results.csv";
};

struct Command {
    std::string name;
    std::vector<std::string> args;
};

static double rseHll(std::uint32_t k) {
    const double m = static_cast<double>(1u << k);
    return 1.04 / std::sqrt(m);
}

static double rseLogLog(std::uint32_t k) {
    const double m = static_cast<double>(1u << k);
    return 1.30 / std::sqrt(m);
}

static double rseUnknown() {
    return std::numeric_limits<double>::quiet_NaN();
}

static void printHelp() {
    std::cout
        << "Comandi disponibili:\n"
        << "  help                         Mostra questo help\n"
        << "  show                         Stampa i parametri correnti\n"
        << "  list                         Elenca algoritmi supportati\n"
        << "  set <param> <value>          Imposta un parametro\n"
        << "  run <algo|all>               Esegue uno o piu' algoritmi\n"
        << "  quit                         Esce\n";
}

static void printConfig(const RunConfig &cfg) {
    std::size_t sampleSize = 0;
    std::size_t runs = 0;
    std::uint32_t seed = 0;
    try {
        const auto index = satp::io::indexBinaryDataset(cfg.datasetPath);
        sampleSize = index.info.elements_per_partition;
        runs = index.info.partition_count;
        seed = index.info.seed;
    } catch (const std::exception &) {
        // Keep defaults below and report file parsing issue in output.
    }

    std::cout
        << "Parametri correnti:\n"
        << "  datasetPath   = " << cfg.datasetPath << '\n'
        << "  sampleSize    = " << sampleSize << " (dal dataset)\n"
        << "  runs          = " << runs << " (dal dataset)\n"
        << "  seed          = " << seed << " (dal dataset)\n"
        << "  k             = " << cfg.k << '\n'
        << "  l             = " << cfg.l << '\n'
        << "  lLog          = " << cfg.lLog << '\n'
        << "  csvPath       = " << cfg.csvPath << '\n';
}

static void printAlgorithms() {
    std::cout
        << "Algoritmi:\n"
        << "  hllpp  (HyperLogLog++)\n"
        << "  hll    (HyperLogLog)\n"
        << "  ll     (LogLog)\n"
        << "  pc     (ProbabilisticCounting)\n";
}

static Command parseCommand(const std::string &line) {
    std::istringstream iss(line);
    Command cmd;
    iss >> cmd.name;
    for (std::string token; iss >> token; ) {
        cmd.args.push_back(token);
    }
    return cmd;
}

static bool setParam(RunConfig &cfg, const std::string &param, const std::string &value) {
    auto toU32 = [](const std::string &s, std::uint32_t &out) {
        std::size_t idx = 0;
        unsigned long v = std::stoul(s, &idx);
        if (idx != s.size() || v > std::numeric_limits<std::uint32_t>::max()) return false;
        out = static_cast<std::uint32_t>(v);
        return true;
    };

    if (param == "datasetPath") {
        cfg.datasetPath = value;
        return true;
    }
    if (param == "k") return toU32(value, cfg.k);
    if (param == "l") return toU32(value, cfg.l);
    if (param == "lLog") return toU32(value, cfg.lLog);
    if (param == "csvPath") {
        cfg.csvPath = value;
        return true;
    }
    return false;
}

static void runAlgorithms(const RunConfig &cfg, const std::vector<std::string> &algs) {
    const auto datasetIndex = satp::io::indexBinaryDataset(cfg.datasetPath);
    const std::size_t sampleSize = datasetIndex.info.elements_per_partition;
    const std::size_t runs = datasetIndex.info.partition_count;
    const std::uint32_t seed = datasetIndex.info.seed;

    eval::EvaluationFramework bench(datasetIndex);

    std::unordered_set<std::string> todo;
    for (const auto &name : algs) todo.insert(name);


    std::cout << "sampleSize: " << sampleSize
              << '\t' << "runs: " << runs
              << '\t' << "seed: " << seed << '\n';


    auto runHllpp = [&]() {
        const std::string params = "k=" + std::to_string(cfg.k);
        const auto stats = bench.evaluateToCsv<alg::HyperLogLogPlusPlus>(
            cfg.csvPath, runs, sampleSize, params, rseHll(cfg.k), cfg.k);
        std::cout << "[HLL++] mean=" << stats.mean
                  << "  f0_hat=" << stats.mean
                  << "  f0_true=" << stats.truth_mean
                  << "  var=" << stats.variance
                  << "  stddev=" << stats.stddev
                  << "  bias=" << stats.bias
                  << "  mre=" << stats.mean_relative_error
                  << "  rmse=" << stats.rmse
                  << "  mae=" << stats.mae << '\n';
    };

    auto runHll = [&]() {
        const std::string params = "k=" + std::to_string(cfg.k) + ",L=" + std::to_string(cfg.lLog);
        const auto stats = bench.evaluateToCsv<alg::HyperLogLog>(
            cfg.csvPath, runs, sampleSize, params, rseHll(cfg.k), cfg.k, cfg.lLog);
        std::cout << "[HLL ] mean=" << stats.mean
                  << "  f0_hat=" << stats.mean
                  << "  f0_true=" << stats.truth_mean
                  << "  var=" << stats.variance
                  << "  stddev=" << stats.stddev
                  << "  bias=" << stats.bias
                  << "  mre=" << stats.mean_relative_error
                  << "  rmse=" << stats.rmse
                  << "  mae=" << stats.mae << '\n';
    };

    auto runLl = [&]() {
        const std::string params = "k=" + std::to_string(cfg.k) + ",L=" + std::to_string(cfg.lLog);
        const auto stats = bench.evaluateToCsv<alg::LogLog>(
            cfg.csvPath, runs, sampleSize, params, rseLogLog(cfg.k), cfg.k, cfg.lLog);
        std::cout << "[LL  ] mean=" << stats.mean
                  << "  f0_hat=" << stats.mean
                  << "  f0_true=" << stats.truth_mean
                  << "  var=" << stats.variance
                  << "  stddev=" << stats.stddev
                  << "  bias=" << stats.bias
                  << "  mre=" << stats.mean_relative_error
                  << "  rmse=" << stats.rmse
                  << "  mae=" << stats.mae << '\n';
    };

    auto runPc = [&]() {
        const std::string params = "L=" + std::to_string(cfg.l);
        const auto stats = bench.evaluateToCsv<alg::ProbabilisticCounting>(
            cfg.csvPath, runs, sampleSize, params, rseUnknown(), cfg.l);
        std::cout << "[PC  ] mean=" << stats.mean
                  << "  f0_hat=" << stats.mean
                  << "  f0_true=" << stats.truth_mean
                  << "  var=" << stats.variance
                  << "  stddev=" << stats.stddev
                  << "  bias=" << stats.bias
                  << "  mre=" << stats.mean_relative_error
                  << "  rmse=" << stats.rmse
                  << "  mae=" << stats.mae << '\n';
    };

    if (todo.count("all")) {
        runHllpp();
        runHll();
        runLl();
        runPc();
        return;
    }

    if (todo.count("hllpp")) runHllpp();
    if (todo.count("hll")) runHll();
    if (todo.count("ll")) runLl();
    if (todo.count("pc")) runPc();
}

int main() {
    RunConfig cfg;
    std::cout << "SATP benchmark CLI. Digita 'help' per i comandi.\n";

    for (std::string line; std::cout << "> " && std::getline(std::cin, line); ) {
        auto cmd = parseCommand(line);
        if (cmd.name.empty()) continue;

        if (cmd.name == "help") {
            printHelp();
            continue;
        }
        if (cmd.name == "show") {
            printConfig(cfg);
            continue;
        }
        if (cmd.name == "list") {
            printAlgorithms();
            continue;
        }
        if (cmd.name == "set") {
            if (cmd.args.size() < 2) {
                std::cout << "Uso: set <param> <value>\n";
                continue;
            }
            if (!setParam(cfg, cmd.args[0], cmd.args[1])) {
                std::cout << "Parametro o valore non valido\n";
            }
            continue;
        }
        if (cmd.name == "run") {
            if (cmd.args.empty()) {
                std::cout << "Uso: run <algo|all>\n";
                continue;
            }
            runAlgorithms(cfg, cmd.args);
            continue;
        }
        if (cmd.name == "quit") {
            break;
        }

        std::cout << "Comando sconosciuto. Digita 'help'.\n";
    }
}
