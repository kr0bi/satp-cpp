#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"
#include "satp/algorithms/NaiveCounting.h"
#include "satp/hashing/HashFactory.h"
#include "satp/simulation/framework/EvaluationFramework.h"
#include "satp/simulation/merge/MergeSummary.h"
#include "satp/simulation/results/CsvResultWriter.h"
#include "satp/simulation/streaming/StreamingCheckpointBuilder.h"
#include "TestData.h"

using namespace std;

namespace eval = satp::evaluation;
namespace alg = satp::algorithms;
using Catch::Approx;

namespace {
    struct EvaluationFrameworkFixture {
        satp::testdata::LoadedDataset dataset = satp::testdata::loadDataset();
        eval::EvaluationFramework bench{satp::testdata::datasetPath(), satp::hashing::getHashFunctionBy()};

        [[nodiscard]] size_t runs() const noexcept {
            return dataset.partition_count;
        }

        [[nodiscard]] size_t sampleSize() const noexcept {
            return dataset.elements_per_partition;
        }
    };

    [[nodiscard]] filesystem::path mergePairsCsvPath() {
        return filesystem::temp_directory_path() / "satp_merge_pairs_test.csv";
    }
} // namespace

TEST_CASE("Evaluation Framework usa sempre i metadata del dataset", "[eval-framework][metadata]") {
    const EvaluationFrameworkFixture fixture;
    const auto &metadata = fixture.bench.metadata();

    REQUIRE(metadata.runs == fixture.runs());
    REQUIRE(metadata.sampleSize == fixture.sampleSize());
    REQUIRE(metadata.distinctCount == fixture.dataset.distinct);
    REQUIRE(metadata.seed == fixture.dataset.seed);
}

TEST_CASE("Evaluation Framework streaming usa F0(t) del dataset", "[eval-framework][streaming]") {
    const EvaluationFrameworkFixture fixture;

    const auto series = fixture.bench.evaluateStreaming<alg::NaiveCounting>();
    const auto expectedCheckpoints = eval::StreamingCheckpointBuilder::build(
        fixture.sampleSize(),
        eval::EvaluationFramework::DEFAULT_STREAMING_CHECKPOINTS);

    REQUIRE(series.size() == expectedCheckpoints.size());
    REQUIRE_FALSE(series.empty());

    for (size_t i = 0; i < series.size(); ++i) {
        const auto &point = series[i];
        const size_t expectedIndex = expectedCheckpoints[i];

        REQUIRE(isfinite(point.mean));
        REQUIRE(isfinite(point.truth_mean));
        REQUIRE(isfinite(point.variance));
        REQUIRE(isfinite(point.stddev));
        REQUIRE(isfinite(point.rmse));
        REQUIRE(isfinite(point.mae));
        REQUIRE(isfinite(point.bias));
        REQUIRE(isfinite(point.mean_relative_error));
        REQUIRE(point.number_of_elements_processed >= 1);
        REQUIRE(point.number_of_elements_processed <= fixture.sampleSize());
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
    REQUIRE(last.truth_mean == Approx(static_cast<double>(fixture.dataset.distinct)).margin(1e-12));
    REQUIRE(last.mean == Approx(static_cast<double>(fixture.dataset.distinct)).margin(1e-12));
}

TEST_CASE("Evaluation Framework streaming notifica il progresso tramite callback", "[eval-framework][streaming][progress]") {
    const EvaluationFrameworkFixture fixture;

    size_t startedWith = 0;
    size_t advancedTicks = 0;
    size_t finishCalls = 0;
    const eval::ProgressCallbacks progress{
        [&](const size_t totalTicks) { startedWith = totalTicks; },
        [&](const size_t ticks) { advancedTicks += ticks; },
        [&]() { ++finishCalls; }
    };

    const auto series = fixture.bench.evaluateStreaming<alg::NaiveCounting>(progress);

    REQUIRE_FALSE(series.empty());
    REQUIRE(startedWith == fixture.runs() * fixture.sampleSize());
    REQUIRE(advancedTicks == fixture.runs() * fixture.sampleSize());
    REQUIRE(finishCalls == 1u);
}

TEST_CASE("Streaming checkpoint builder usa fasi percentuali e termina a n", "[eval-framework][streaming]") {
    constexpr size_t n = 10'000'000u;
    constexpr size_t maxPoints = eval::EvaluationFramework::DEFAULT_STREAMING_CHECKPOINTS;

    const auto checkpoints = eval::StreamingCheckpointBuilder::build(n, maxPoints);

    REQUIRE_FALSE(checkpoints.empty());
    REQUIRE(checkpoints.front() == 1u);
    REQUIRE(checkpoints.back() == n);
    REQUIRE(checkpoints.size() <= maxPoints);

    for (size_t i = 1; i < checkpoints.size(); ++i) {
        REQUIRE(checkpoints[i] > checkpoints[i - 1]);
    }

    // First phase (dense) covers 0.1% of n; there should be many early checkpoints.
    const size_t phase1End = static_cast<size_t>(ceil(static_cast<double>(n) * 1e-3));
    const size_t phase2End = static_cast<size_t>(ceil(static_cast<double>(n) * 1e-1));
    const size_t inPhase1 = static_cast<size_t>(count_if(
        checkpoints.begin(),
        checkpoints.end(),
        [phase1End](const size_t v) { return v <= phase1End; }));
    const size_t inPhase12 = static_cast<size_t>(count_if(
        checkpoints.begin(),
        checkpoints.end(),
        [phase2End](const size_t v) { return v <= phase2End; }));

    REQUIRE(inPhase1 >= maxPoints / 4u);
    REQUIRE(inPhase12 >= maxPoints / 2u);
}

TEST_CASE("Evaluation Framework merge pairs: Naive merge equivale al seriale", "[eval-framework][merge]") {
    const EvaluationFrameworkFixture fixture;

    const auto points = fixture.bench.evaluateMergePairs<alg::NaiveCounting>();
    REQUIRE(points.size() == fixture.runs() / 2u);
    REQUIRE_FALSE(points.empty());

    for (const auto &point : points) {
        REQUIRE(isfinite(point.estimate_merge));
        REQUIRE(isfinite(point.estimate_serial));
        REQUIRE(isfinite(point.delta_merge_serial_abs));
        REQUIRE(isfinite(point.delta_merge_serial_rel));
        REQUIRE(point.delta_merge_serial_abs == Approx(0.0).margin(1e-12));
        REQUIRE(point.delta_merge_serial_rel == Approx(0.0).margin(1e-12));
    }
}

TEST_CASE("Evaluation Framework merge notifica il progresso tramite callback", "[eval-framework][merge][progress]") {
    const EvaluationFrameworkFixture fixture;

    size_t startedWith = 0;
    size_t advancedTicks = 0;
    size_t finishCalls = 0;
    const eval::ProgressCallbacks progress{
        [&](const size_t totalTicks) { startedWith = totalTicks; },
        [&](const size_t ticks) { advancedTicks += ticks; },
        [&]() { ++finishCalls; }
    };

    const auto points = fixture.bench.evaluateMergePairs<alg::NaiveCounting>(progress);

    REQUIRE(points.size() == fixture.runs() / 2u);
    REQUIRE(startedWith == (fixture.runs() / 2u) * fixture.sampleSize() * 4u);
    REQUIRE(advancedTicks == (fixture.runs() / 2u) * fixture.sampleSize() * 4u);
    REQUIRE(finishCalls == 1u);
}

TEST_CASE("Evaluation Framework merge pairs CSV", "[eval-framework][merge][csv]") {
    namespace fs = filesystem;

    const EvaluationFrameworkFixture fixture;

    const fs::path csvPath = mergePairsCsvPath();
    if (fs::exists(csvPath)) {
        fs::remove(csvPath);
    }

    const auto points = fixture.bench.evaluateMergePairs<alg::NaiveCounting>();
    const eval::CsvRunDescriptor descriptor{
        "Naive",
        "naive",
        fixture.bench.metadata()
    };
    eval::CsvResultWriter::appendMergePairs(csvPath, descriptor, points);
    const auto stats = eval::summarizeMergePairs(points);

    REQUIRE(stats.pair_count == fixture.runs() / 2u);
    REQUIRE(stats.delta_merge_serial_abs_mean == Approx(0.0).margin(1e-12));
    REQUIRE(stats.delta_merge_serial_abs_max == Approx(0.0).margin(1e-12));
    REQUIRE(stats.delta_merge_serial_rmse == Approx(0.0).margin(1e-12));

    REQUIRE(fs::exists(csvPath));
    ifstream in(csvPath);
    REQUIRE(in.good());
    string header;
    getline(in, header);
    REQUIRE(header.find("estimate_merge") != string::npos);
    REQUIRE(header.find("estimate_serial") != string::npos);

    fs::remove(csvPath);
}
