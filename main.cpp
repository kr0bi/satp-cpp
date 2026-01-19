#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

#include "satp/io/FileNaming.h"
#include "src/satp/Utils.h"
#include "src/satp/algorithms/HyperLogLog.h"
#include "src/satp/algorithms/HyperLogLogPlusPlus.h"
#include "src/satp/algorithms/LogLog.h"
#include "src/satp/algorithms/ProbabilisticCounting.h"
#include "src/satp/simulation/EvaluationFramework.h"

namespace eval = satp::evaluation;
namespace alg = satp::algorithms;
namespace util = satp::utils;

struct RunConfig {
    std::size_t highestNumber = 1'000'000'000;
    std::size_t numberOfElems = 100'000'000;
    std::size_t sampleSize = 1'000'000;
    std::size_t runs = 10;
    std::uint32_t seed = eval::EvaluationFramework::DEFAULT_SEED;

    std::uint32_t k = 16; // registri per HLL/LL
    std::uint32_t l = 16; // bitmap PC
    std::uint32_t lLog = 32; // bitmap LL

    std::string csvPath = "results.csv";
};

struct Command {
    std::string name;
    std::vector<std::string> args;
};

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
    std::cout
        << "Parametri correnti:\n"
        << "  highestNumber = " << cfg.highestNumber << '\n'
        << "  numberOfElems = " << cfg.numberOfElems << '\n'
        << "  sampleSize    = " << cfg.sampleSize << '\n'
        << "  runs          = " << cfg.runs << '\n'
        << "  seed          = " << cfg.seed << '\n'
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
    auto toSizeT = [](const std::string &s, std::size_t &out) {
        std::size_t idx = 0;
        out = std::stoull(s, &idx);
        return idx == s.size();
    };
    auto toU32 = [](const std::string &s, std::uint32_t &out) {
        std::size_t idx = 0;
        unsigned long v = std::stoul(s, &idx);
        if (idx != s.size() || v > std::numeric_limits<std::uint32_t>::max()) return false;
        out = static_cast<std::uint32_t>(v);
        return true;
    };

    if (param == "highestNumber") return toSizeT(value, cfg.highestNumber);
    if (param == "numberOfElems") return toSizeT(value, cfg.numberOfElems);
    if (param == "sampleSize") return toSizeT(value, cfg.sampleSize);
    if (param == "runs") return toSizeT(value, cfg.runs);
    if (param == "seed") return toU32(value, cfg.seed);
    if (param == "k") return toU32(value, cfg.k);
    if (param == "l") return toU32(value, cfg.l);
    if (param == "lLog") return toU32(value, cfg.lLog);
    if (param == "csvPath") {
        cfg.csvPath = value;
        return true;
    }
    return false;
}

static std::string datasetFilename(const RunConfig &cfg) {
    return satp::io::makeDatasetFilename(cfg.numberOfElems,
                                         cfg.highestNumber,
                                         cfg.sampleSize,
                                         cfg.runs);
}

static void runAlgorithms(const RunConfig &cfg, const std::vector<std::string> &algs) {
    const std::string cacheFile = datasetFilename(cfg);
    eval::EvaluationFramework bench(
        cacheFile,
        cfg.runs,
        cfg.sampleSize,
        cfg.numberOfElems,
        cfg.highestNumber,
        cfg.seed);

    std::unordered_set<std::string> todo;
    for (const auto &name : algs) todo.insert(name);

    auto runHllpp = [&]() {
        const std::string params = "k=" + std::to_string(cfg.k);
        const auto stats = bench.evaluateToCsv<alg::HyperLogLogPlusPlus>(
            cfg.csvPath, cfg.runs, cfg.sampleSize, params, cfg.k);
        std::cout << "[HLL++] mean=" << stats.mean
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
            cfg.csvPath, cfg.runs, cfg.sampleSize, params, cfg.k, cfg.lLog);
        std::cout << "[HLL ] mean=" << stats.mean
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
            cfg.csvPath, cfg.runs, cfg.sampleSize, params, cfg.k, cfg.lLog);
        std::cout << "[LL  ] mean=" << stats.mean
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
            cfg.csvPath, cfg.runs, cfg.sampleSize, params, cfg.l);
        std::cout << "[PC  ] mean=" << stats.mean
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
