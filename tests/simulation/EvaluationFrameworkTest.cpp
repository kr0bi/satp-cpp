#include <iostream>

#include "catch2/catch_test_macros.hpp"
#include "satp/Utils.h"
#include "satp/algorithms/HyperLogLog.h"
#include "satp/algorithms/LogLog.h"
#include "satp/algorithms/ProbabilisticCounting.h"
#include "satp/simulation/Loop.h"
#include "satp/simulation/EvaluationFramework.h"


namespace eval = satp::evaluation;
namespace alg = satp::algorithms;
namespace util = satp::utils;


TEST_CASE("Evaluation Framework", "[eval-framework]") {
        // --------------- parametri del benchmark ---------------------------
        constexpr std::size_t HIGHEST_NUMBER = 100'000'000;
        constexpr std::size_t NUMBER_OF_ELEMS = 1'000'000'000;
        constexpr std::size_t SAMPLE_SIZE = 1'000'000;
        constexpr std::size_t RUNS = 50;

        constexpr std::uint32_t K = 16; // registri LogLog
        constexpr std::uint32_t L = 16; // bitmap ProbabilisticCounting
        constexpr std::uint32_t L_LOG = 32; // bitmap LogLog

        // --------------- dataset generato una volta sola -------------------
        auto data = util::getRandomNumbers(NUMBER_OF_ELEMS, HIGHEST_NUMBER);

        eval::EvaluationFramework bench(std::move(data));

        std::cout << "Ground‑truth distinct = " << bench.getNumElementiDistintiEffettivi() << '\n';

        // -------- valutazione HyperLogLog ----------------------------------
        auto hllStats = bench.evaluate<alg::HyperLogLog>(RUNS, SAMPLE_SIZE, K, L_LOG);
        std::cout << "[HyperLogLog]  mean=" << hllStats.mean
                        << "  var=" << hllStats.variance
                        << "  bias=" << hllStats.bias << '\n';

        // -------- valutazione LogLog ---------------------------------------
        auto llStats = bench.evaluate<alg::LogLog>(RUNS, SAMPLE_SIZE, K, L_LOG);

        std::cout << "[LogLog]  mean=" << llStats.mean
                        << "  var=" << llStats.variance
                        << "  bias=" << llStats.bias << '\n';

        // -------- valutazione ProbabilisticCounting ------------------------
        auto pcStats = bench.evaluate<alg::ProbabilisticCounting>(RUNS, SAMPLE_SIZE, L);

        std::cout << "[PC]      mean=" << pcStats.mean
                        << "  var=" << pcStats.variance
                        << "  bias=" << pcStats.bias << '\n';
}
