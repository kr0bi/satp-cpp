#include <cmath>
#include <iostream>

#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"
#include "satp/algorithms/HyperLogLog.h"
#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/algorithms/LogLog.h"
#include "satp/algorithms/NaiveCounting.h"
#include "satp/algorithms/ProbabilisticCounting.h"
#include "satp/simulation/Loop.h"
#include "satp/simulation/EvaluationFramework.h"
#include "TestData.h"


namespace eval = satp::evaluation;
namespace alg = satp::algorithms;
using Catch::Approx;

static void requireFiniteNonNegative(const eval::Stats &stats) {
    REQUIRE(std::isfinite(stats.mean));
    REQUIRE(std::isfinite(stats.variance));
    REQUIRE(std::isfinite(stats.stddev));
    REQUIRE(std::isfinite(stats.rmse));
    REQUIRE(std::isfinite(stats.mae));
    REQUIRE(std::isfinite(stats.mean_relative_error));
    REQUIRE(std::isfinite(stats.bias));
    REQUIRE(std::isfinite(stats.bias_relative));
    REQUIRE(stats.variance >= 0.0);
    REQUIRE(stats.stddev >= 0.0);
    REQUIRE(stats.rmse >= 0.0);
    REQUIRE(stats.mae >= 0.0);
    REQUIRE(stats.mean_relative_error >= 0.0);
    REQUIRE(stats.difference >= 0.0);
    REQUIRE(std::abs(stats.difference - std::abs(stats.bias)) < 1e-12);
}

TEST_CASE("Evaluation Framework", "[eval-framework]") {
        // --------------- parametri del benchmark ---------------------------
        constexpr std::uint32_t K = 16; // registri LogLog
        constexpr std::uint32_t L = 16; // bitmap ProbabilisticCounting
        constexpr std::uint32_t L_LOG = 32; // bitmap LogLog

        // --------------- dataset da file ----------------------------------
        eval::EvaluationFramework bench(satp::testdata::datasetPath());
        const auto dataset = satp::testdata::loadDataset();
        const auto SAMPLE_SIZE = dataset.elements_per_partition;
        const auto RUNS = dataset.partition_count;

        std::cout << "Ground‑truth distinct = " << bench.getNumElementiDistintiEffettivi() << '\n';

        // -------- valutazione HyperLogLog ----------------------------------
        auto hllStats = bench.evaluate<alg::HyperLogLogPlusPlus>(RUNS, SAMPLE_SIZE, K);
        std::cout << "[HyperLogLog]  mean=" << hllStats.mean
                        << "  var=" << hllStats.variance
                        << "  bias=" << hllStats.bias << '\n';
        requireFiniteNonNegative(hllStats);

        // -------- valutazione LogLog ---------------------------------------
        auto llStats = bench.evaluate<alg::LogLog>(RUNS, SAMPLE_SIZE, K, L_LOG);

        std::cout << "[LogLog]  mean=" << llStats.mean
                        << "  var=" << llStats.variance
                        << "  bias=" << llStats.bias << '\n';
        requireFiniteNonNegative(llStats);

        // -------- valutazione ProbabilisticCounting ------------------------
        auto pcStats = bench.evaluate<alg::ProbabilisticCounting>(RUNS, SAMPLE_SIZE, L);

        std::cout << "[PC]      mean=" << pcStats.mean
                        << "  var=" << pcStats.variance
                        << "  bias=" << pcStats.bias << '\n';
        requireFiniteNonNegative(pcStats);
}

TEST_CASE("Evaluation Framework streaming usa F0(t) del dataset", "[eval-framework][streaming]") {
    eval::EvaluationFramework bench(satp::testdata::datasetPath());
    const auto dataset = satp::testdata::loadDataset();
    const auto sampleSize = dataset.elements_per_partition;
    const auto runs = dataset.partition_count;

    const auto series = bench.evaluateStreaming<alg::NaiveCounting>(runs, sampleSize);
    REQUIRE(series.size() == sampleSize);
    REQUIRE_FALSE(series.empty());

    for (const auto &point : series) {
        REQUIRE(std::isfinite(point.mean));
        REQUIRE(std::isfinite(point.truth_mean));
        REQUIRE(std::isfinite(point.variance));
        REQUIRE(std::isfinite(point.stddev));
        REQUIRE(std::isfinite(point.rmse));
        REQUIRE(std::isfinite(point.mae));
        REQUIRE(std::isfinite(point.bias));
        REQUIRE(std::isfinite(point.mean_relative_error));
        REQUIRE(point.element_index >= 1);
        REQUIRE(point.element_index <= sampleSize);

        // NaiveCounting e' esatto: a ogni prefisso stima == F0(t) run-per-run.
        REQUIRE(point.bias == Approx(0.0).margin(1e-12));
        REQUIRE(point.difference == Approx(0.0).margin(1e-12));
        REQUIRE(point.rmse == Approx(0.0).margin(1e-12));
        REQUIRE(point.mae == Approx(0.0).margin(1e-12));
        REQUIRE(point.mean_relative_error == Approx(0.0).margin(1e-12));
    }

    const auto &last = series.back();
    REQUIRE(last.truth_mean == Approx(static_cast<double>(dataset.distinct)).margin(1e-12));
    REQUIRE(last.mean == Approx(static_cast<double>(dataset.distinct)).margin(1e-12));
}
