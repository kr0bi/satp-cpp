#include <iostream>

#include "catch2/catch_test_macros.hpp"
#include "satp/algorithms/HyperLogLog.h"
#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/algorithms/LogLog.h"
#include "satp/algorithms/ProbabilisticCounting.h"
#include "satp/simulation/Loop.h"
#include "satp/simulation/EvaluationFramework.h"
#include "TestData.h"


namespace eval = satp::evaluation;
namespace alg = satp::algorithms;


TEST_CASE("Evaluation Framework", "[eval-framework]") {
        // --------------- parametri del benchmark ---------------------------
        constexpr std::size_t SAMPLE_SIZE = 2'000;
        constexpr std::size_t RUNS = 3;

        constexpr std::uint32_t K = 16; // registri LogLog
        constexpr std::uint32_t L = 16; // bitmap ProbabilisticCounting
        constexpr std::uint32_t L_LOG = 32; // bitmap LogLog

        // --------------- dataset da file ----------------------------------
        eval::EvaluationFramework bench(satp::testdata::datasetPath());

        std::cout << "Ground‑truth distinct = " << bench.getNumElementiDistintiEffettivi() << '\n';

        // -------- valutazione HyperLogLog ----------------------------------
        auto hllStats = bench.evaluate<alg::HyperLogLogPlusPlus>(RUNS, SAMPLE_SIZE, K);
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
