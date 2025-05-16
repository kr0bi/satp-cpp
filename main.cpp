// main.cpp ---------------------------------------------------------------
#include <iostream>
#include "src/satp/simulation/EvaluationFramework.h"
#include "src/satp/Utils.h"
#include "src/satp/algorithms/LogLog.h"
#include "src/satp/algorithms/ProbabilisticCounting.h"

namespace eval = satp::evaluation;
namespace alg = satp::algorithms;
namespace util = satp::utils;

int main() {
    // --------------- parametri del benchmark ---------------------------
    constexpr std::size_t HIGHEST_NUMBER = 100'000;
    constexpr std::size_t NUMBER_OF_ELEMS = 10'000'00;
    constexpr std::size_t RUNS = 50;

    constexpr std::uint32_t K = 16; // registri LogLog
    constexpr std::uint32_t L = 16; // bitmap ProbabilisticCounting

    // --------------- dataset generato una volta sola -------------------
    auto data = util::getRandomNumbers(NUMBER_OF_ELEMS, HIGHEST_NUMBER);

    eval::EvaluationFramework bench(std::move(data));

    std::cout << "Ground‑truth distinct = " << bench.ground_truth() << '\n';

    // -------- valutazione LogLog ---------------------------------------
    auto llStats = bench.evaluate<alg::LogLog>(RUNS, K /*, altri ctor arg*/);

    std::cout << "[LogLog]  mean=" << llStats.mean
            << "  var=" << llStats.variance
            << "  bias=" << llStats.bias << '\n';

    // -------- valutazione ProbabilisticCounting ------------------------
    auto pcStats = bench.evaluate<alg::ProbabilisticCounting>(RUNS, L);

    std::cout << "[PC]      mean=" << pcStats.mean
            << "  var=" << pcStats.variance
            << "  bias=" << pcStats.bias << '\n';

    return 0;
}
