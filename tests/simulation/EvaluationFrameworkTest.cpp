#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
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
#include "satp/simulation/StreamingCheckpointBuilder.h"
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
    REQUIRE(std::isfinite(stats.relative_bias));
    REQUIRE(stats.variance >= 0.0);
    REQUIRE(stats.stddev >= 0.0);
    REQUIRE(stats.rmse >= 0.0);
    REQUIRE(stats.mae >= 0.0);
    REQUIRE(stats.mean_relative_error >= 0.0);
    REQUIRE(stats.absolute_bias >= 0.0);
    REQUIRE(std::abs(stats.absolute_bias - std::abs(stats.bias)) < 1e-12);
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
    const auto expectedCheckpoints = eval::StreamingCheckpointBuilder::build(
        sampleSize,
        eval::EvaluationFramework::DEFAULT_STREAMING_CHECKPOINTS);

    REQUIRE(series.size() == expectedCheckpoints.size());
    REQUIRE_FALSE(series.empty());

    for (size_t i = 0; i < series.size(); ++i) {
        const auto &point = series[i];
        const size_t expectedIndex = expectedCheckpoints[i];

        REQUIRE(std::isfinite(point.mean));
        REQUIRE(std::isfinite(point.truth_mean));
        REQUIRE(std::isfinite(point.variance));
        REQUIRE(std::isfinite(point.stddev));
        REQUIRE(std::isfinite(point.rmse));
        REQUIRE(std::isfinite(point.mae));
        REQUIRE(std::isfinite(point.bias));
        REQUIRE(std::isfinite(point.mean_relative_error));
        REQUIRE(point.number_of_elements_processed >= 1);
        REQUIRE(point.number_of_elements_processed <= sampleSize);
        REQUIRE(point.number_of_elements_processed == expectedIndex);
        if (i > 0) {
            REQUIRE(point.number_of_elements_processed > series[i - 1].number_of_elements_processed);
        }

        // NaiveCounting e' esatto: a ogni prefisso stima == F0(t) run-per-run.
        REQUIRE(point.bias == Approx(0.0).margin(1e-12));
        REQUIRE(point.absolute_bias == Approx(0.0).margin(1e-12));
        REQUIRE(point.rmse == Approx(0.0).margin(1e-12));
        REQUIRE(point.mae == Approx(0.0).margin(1e-12));
        REQUIRE(point.mean_relative_error == Approx(0.0).margin(1e-12));
    }

    const auto &last = series.back();
    REQUIRE(last.truth_mean == Approx(static_cast<double>(dataset.distinct)).margin(1e-12));
    REQUIRE(last.mean == Approx(static_cast<double>(dataset.distinct)).margin(1e-12));
}

TEST_CASE("Streaming checkpoint builder usa fasi percentuali e termina a n", "[eval-framework][streaming]") {
    constexpr std::size_t n = 10'000'000u;
    constexpr std::size_t maxPoints = eval::EvaluationFramework::DEFAULT_STREAMING_CHECKPOINTS;

    const auto checkpoints = eval::StreamingCheckpointBuilder::build(n, maxPoints);

    REQUIRE_FALSE(checkpoints.empty());
    REQUIRE(checkpoints.front() == 1u);
    REQUIRE(checkpoints.back() == n);
    REQUIRE(checkpoints.size() <= maxPoints);

    for (std::size_t i = 1; i < checkpoints.size(); ++i) {
        REQUIRE(checkpoints[i] > checkpoints[i - 1]);
    }

    // First phase (dense) covers 0.1% of n; there should be many early checkpoints.
    const std::size_t phase1End = static_cast<std::size_t>(std::ceil(static_cast<double>(n) * 1e-3));
    const std::size_t phase2End = static_cast<std::size_t>(std::ceil(static_cast<double>(n) * 1e-1));
    const std::size_t inPhase1 = static_cast<std::size_t>(std::count_if(
        checkpoints.begin(),
        checkpoints.end(),
        [phase1End](const std::size_t v) { return v <= phase1End; }));
    const std::size_t inPhase12 = static_cast<std::size_t>(std::count_if(
        checkpoints.begin(),
        checkpoints.end(),
        [phase2End](const std::size_t v) { return v <= phase2End; }));

    REQUIRE(inPhase1 >= maxPoints / 4u);
    REQUIRE(inPhase12 >= maxPoints / 2u);
}

TEST_CASE("Evaluation Framework merge pairs: Naive merge equivale al seriale", "[eval-framework][merge]") {
    eval::EvaluationFramework bench(satp::testdata::datasetPath());
    const auto dataset = satp::testdata::loadDataset();

    const auto points = bench.evaluateMergePairs<alg::NaiveCounting>(
        dataset.partition_count,
        dataset.elements_per_partition);
    REQUIRE(points.size() == dataset.partition_count / 2u);
    REQUIRE_FALSE(points.empty());

    for (const auto &point : points) {
        REQUIRE(std::isfinite(point.estimate_merge));
        REQUIRE(std::isfinite(point.estimate_serial));
        REQUIRE(std::isfinite(point.delta_merge_serial_abs));
        REQUIRE(std::isfinite(point.delta_merge_serial_rel));
        REQUIRE(point.delta_merge_serial_abs == Approx(0.0).margin(1e-12));
        REQUIRE(point.delta_merge_serial_rel == Approx(0.0).margin(1e-12));
    }
}

TEST_CASE("Evaluation Framework merge pairs CSV", "[eval-framework][merge][csv]") {
    namespace fs = std::filesystem;

    eval::EvaluationFramework bench(satp::testdata::datasetPath());
    const auto dataset = satp::testdata::loadDataset();

    const fs::path csvPath = fs::temp_directory_path() / "satp_merge_pairs_test.csv";
    if (fs::exists(csvPath)) {
        fs::remove(csvPath);
    }

    const auto stats = bench.evaluateMergePairsToCsv<alg::NaiveCounting>(
        csvPath,
        dataset.partition_count,
        dataset.elements_per_partition,
        "naive");

    REQUIRE(stats.pair_count == dataset.partition_count / 2u);
    REQUIRE(stats.delta_merge_serial_abs_mean == Approx(0.0).margin(1e-12));
    REQUIRE(stats.delta_merge_serial_abs_max == Approx(0.0).margin(1e-12));
    REQUIRE(stats.delta_merge_serial_rmse == Approx(0.0).margin(1e-12));

    REQUIRE(fs::exists(csvPath));
    std::ifstream in(csvPath);
    REQUIRE(in.good());
    std::string header;
    std::getline(in, header);
    REQUIRE(header.find("estimate_merge") != std::string::npos);
    REQUIRE(header.find("estimate_serial") != std::string::npos);

    fs::remove(csvPath);
}
