#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"
#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/algorithms/NaiveCounting.h"
#include "satp/hashing/HashFactory.h"
#include "satp/simulation/Simulation.h"
#include "TestData.h"

using namespace std;

namespace eval = satp::evaluation;
namespace alg = satp::algorithms;
using Catch::Approx;

namespace {
    [[nodiscard]] uint32_t parseSingleUnsignedParam(const string &params,
                                                    const string &key) {
        const string prefix = key + "=";
        const size_t pos = params.find(prefix);
        if (pos == string::npos) {
            throw invalid_argument("Parametro mancante: " + key);
        }

        size_t end = params.find(',', pos);
        if (end == string::npos) end = params.size();
        return static_cast<uint32_t>(stoul(params.substr(pos + prefix.size(), end - (pos + prefix.size()))));
    }

    [[nodiscard]] alg::HyperLogLogPlusPlus buildHllppFromContext(
        const eval::MergeSketchContext &context,
        const satp::hashing::HashFunction &hashFunction) {
        return alg::HyperLogLogPlusPlus(parseSingleUnsignedParam(context.params, "k"), hashFunction);
    }

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

    [[nodiscard]] filesystem::path heterogeneousMergeCsvPath() {
        return filesystem::temp_directory_path() / "satp_merge_heterogeneous_test.csv";
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
    const auto expectedCheckpoints = eval::CheckpointPlanner::build(
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

TEST_CASE("Checkpoint planner rispetta il budget e copre l'intero stream", "[eval-framework][streaming]") {
    constexpr size_t n = 10'000'000u;
    constexpr size_t maxPoints = eval::EvaluationFramework::DEFAULT_STREAMING_CHECKPOINTS;

    const auto checkpoints = eval::CheckpointPlanner::build(n, maxPoints);

    REQUIRE_FALSE(checkpoints.empty());
    REQUIRE(checkpoints.front() == 1u);
    REQUIRE(checkpoints.back() == n);
    REQUIRE(checkpoints.size() == maxPoints);

    for (size_t i = 1; i < checkpoints.size(); ++i) {
        REQUIRE(checkpoints[i] > checkpoints[i - 1]);
    }

    const size_t earlyRangeEnd = static_cast<size_t>(ceil(static_cast<double>(n) * 1e-2));
    const size_t lateRangeStart = static_cast<size_t>(ceil(static_cast<double>(n) * 1e-1));
    const size_t earlyPoints = static_cast<size_t>(count_if(
        checkpoints.begin(),
        checkpoints.end(),
        [earlyRangeEnd](const size_t value) { return value <= earlyRangeEnd; }));
    const size_t latePoints = static_cast<size_t>(count_if(
        checkpoints.begin(),
        checkpoints.end(),
        [lateRangeStart](const size_t value) { return value >= lateRangeStart; }));

    REQUIRE(earlyPoints >= maxPoints / 5u);
    REQUIRE(latePoints >= maxPoints / 4u);
}

TEST_CASE("Checkpoint planner non sfora il budget su stream piccoli o budget piccoli", "[eval-framework][streaming]") {
    const auto tinyBudget = eval::CheckpointPlanner::build(1001u, 2u);
    REQUIRE(tinyBudget == vector<size_t>{1u, 1001u});

    const auto verySmallBudget = eval::CheckpointPlanner::build(10'000'000u, 3u);
    REQUIRE(verySmallBudget.size() == 3u);
    REQUIRE(verySmallBudget.front() == 1u);
    REQUIRE(verySmallBudget.back() == 10'000'000u);

    const auto moderateStream = eval::CheckpointPlanner::build(1'000u, 200u);
    REQUIRE(moderateStream.size() == 200u);
    REQUIRE(moderateStream.front() == 1u);
    REQUIRE(moderateStream.back() == 1'000u);
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

TEST_CASE("CsvResultWriter serializza il CSV del merge eterogeneo", "[eval-framework][merge][heterogeneous][csv]") {
    namespace fs = filesystem;

    const fs::path csvPath = heterogeneousMergeCsvPath();
    if (fs::exists(csvPath)) {
        fs::remove(csvPath);
    }

    const eval::HeterogeneousMergeCsvDescriptor descriptor{
        "HyperLogLog++",
        {"splitmix64", 111u, "k=14"},
        {"xxhash64", 222u, "k=10"},
        eval::MergeStrategy::ReduceThenMerge,
        eval::MergeValidity::Recoverable,
        eval::MergeTopology::Pairwise,
        {
            50u,
            1'000'000u,
            100'000u,
            21041998u
        }
    };
    const vector<eval::HeterogeneousMergePoint> points{
        {
            0u,
            12'345.0,
            12'301.0,
            12'320.0,
            44.0,
            44.0 / 12'345.0,
            25.0,
            25.0 / 12'345.0,
            12'299.0,
            2.0
        }
    };

    eval::CsvResultWriter::appendHeterogeneousMergePairs(csvPath, descriptor, points);

    REQUIRE(fs::exists(csvPath));
    ifstream in(csvPath);
    REQUIRE(in.good());

    string header;
    getline(in, header);
    REQUIRE(header == eval::CsvResultWriter::HETEROGENEOUS_MERGE_HEADER);
    REQUIRE(header.find("left_hash") != string::npos);
    REQUIRE(header.find("baseline_homogeneous") != string::npos);

    string row;
    getline(in, row);
    REQUIRE(row.find("HyperLogLog++") != string::npos);
    REQUIRE(row.find("merge_heterogeneous") != string::npos);
    REQUIRE(row.find("splitmix64") != string::npos);
    REQUIRE(row.find("xxhash64") != string::npos);
    REQUIRE(row.find("reduce_then_merge") != string::npos);
    REQUIRE(row.find("recoverable") != string::npos);
    REQUIRE(row.find("pairwise") != string::npos);

    fs::remove(csvPath);
}

TEST_CASE("Evaluation Framework merge eterogeneo diretto coincide con la baseline omogenea", "[eval-framework][merge][heterogeneous]") {
    const EvaluationFrameworkFixture fixture;

    eval::HeterogeneousMergeRunDescriptor descriptor{
        "HyperLogLog++",
        {"splitmix64", 0u, "k=14"},
        {"splitmix64", 0u, "k=14"},
        eval::MergeStrategy::Direct,
        eval::MergeValidity::Valid,
        eval::MergeTopology::Pairwise,
        fixture.bench.metadata(),
        nullopt,
        eval::MergeSketchContext{"splitmix64", 0u, "k=14"}
    };

    const auto points = fixture.bench.evaluateHeterogeneousMergePairs<alg::HyperLogLogPlusPlus>(
        descriptor,
        buildHllppFromContext);

    REQUIRE(points.size() == fixture.runs() / 2u);
    REQUIRE_FALSE(points.empty());

    for (const auto &point : points) {
        REQUIRE(isfinite(point.exact_union));
        REQUIRE(isfinite(point.estimate_merge));
        REQUIRE(isfinite(point.estimate_serial));
        REQUIRE(isfinite(point.error_merge_abs_exact));
        REQUIRE(isfinite(point.error_serial_abs_exact));
        REQUIRE(isfinite(point.baseline_homogeneous));
        REQUIRE(point.delta_vs_baseline == Approx(0.0).margin(1e-12));
    }
}

TEST_CASE("Evaluation Framework merge eterogeneo reject preserva seriale ed esatto", "[eval-framework][merge][heterogeneous]") {
    const EvaluationFrameworkFixture fixture;

    eval::HeterogeneousMergeRunDescriptor descriptor{
        "HyperLogLog++",
        {"splitmix64", 0u, "k=14"},
        {"xxhash64", 123u, "k=14"},
        eval::MergeStrategy::Reject,
        eval::MergeValidity::Invalid,
        eval::MergeTopology::Pairwise,
        fixture.bench.metadata()
    };

    const auto points = fixture.bench.evaluateHeterogeneousMergePairs<alg::HyperLogLogPlusPlus>(
        descriptor,
        buildHllppFromContext);

    REQUIRE(points.size() == fixture.runs() / 2u);
    REQUIRE_FALSE(points.empty());

    for (const auto &point : points) {
        REQUIRE(isfinite(point.exact_union));
        REQUIRE(isfinite(point.estimate_serial));
        REQUIRE(isnan(point.estimate_merge));
        REQUIRE(isnan(point.error_merge_abs_exact));
        REQUIRE(isnan(point.error_merge_rel_exact));
        REQUIRE(isnan(point.baseline_homogeneous));
        REQUIRE(isnan(point.delta_vs_baseline));
    }
}

TEST_CASE("Evaluation Framework merge eterogeneo segnala reduce_then_merge non implementato", "[eval-framework][merge][heterogeneous]") {
    const EvaluationFrameworkFixture fixture;

    eval::HeterogeneousMergeRunDescriptor descriptor{
        "HyperLogLog++",
        {"splitmix64", 0u, "k=14"},
        {"splitmix64", 0u, "k=10"},
        eval::MergeStrategy::ReduceThenMerge,
        eval::MergeValidity::Recoverable,
        eval::MergeTopology::Pairwise,
        fixture.bench.metadata()
    };

    REQUIRE_THROWS_AS(
        fixture.bench.evaluateHeterogeneousMergePairs<alg::HyperLogLogPlusPlus>(
            descriptor,
            buildHllppFromContext),
        logic_error);
}
