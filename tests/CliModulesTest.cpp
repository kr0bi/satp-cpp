#include <array>
#include <cmath>
#include <string_view>
#include <vector>

#include "catch2/catch_test_macros.hpp"

#include "satp/cli/config/CommandParser.h"
#include "satp/cli/config/RunConfigParams.h"
#include "satp/cli/executor/Output.h"
#include "satp/cli/executor/Selection.h"

using namespace std;

TEST_CASE("CLI command parser parses command tokens", "[cli][config]") {
    {
        const auto cmd = satp::cli::config::parseCommand("set k 16");
        REQUIRE(cmd.name == "set");
        REQUIRE(cmd.args == vector<string>{"k", "16"});
    }

    {
        const auto cmd = satp::cli::config::parseCommand("runstream   hllpp   ll");
        REQUIRE(cmd.name == "runstream");
        REQUIRE(cmd.args == vector<string>{"hllpp", "ll"});
    }

    {
        const auto cmd = satp::cli::config::parseCommand("   ");
        REQUIRE(cmd.name.empty());
        REQUIRE(cmd.args.empty());
    }
}

TEST_CASE("CLI run config parameter setters update and validate values", "[cli][config]") {
    satp::cli::RunConfig cfg;

    REQUIRE(satp::cli::config::setParam(cfg, "datasetPath", "datasets/my.bin"));
    REQUIRE(cfg.datasetPath == "datasets/my.bin");

    REQUIRE(satp::cli::config::setParam(cfg, "resultsNamespace", "nsA"));
    REQUIRE(cfg.resultsNamespace == "nsA");

    REQUIRE(satp::cli::config::setParam(cfg, "hashFunction", "murmur3"));
    REQUIRE(cfg.hashFunctionName == "murmurhash3");

    REQUIRE(satp::cli::config::setParam(cfg, "k", "12"));
    REQUIRE(cfg.k == 12u);

    REQUIRE(satp::cli::config::setParam(cfg, "l", "20"));
    REQUIRE(cfg.l == 20u);

    REQUIRE(satp::cli::config::setParam(cfg, "lLog", "24"));
    REQUIRE(cfg.lLog == 24u);

    const uint32_t oldK = cfg.k;
    REQUIRE_FALSE(satp::cli::config::setParam(cfg, "k", "abc"));
    REQUIRE(cfg.k == oldK);

    REQUIRE_FALSE(satp::cli::config::setParam(cfg, "hashFunction", "not-a-hash"));
    REQUIRE_FALSE(satp::cli::config::setParam(cfg, "unknownParam", "x"));
}

TEST_CASE("CLI run config exposes canonical parameter and hash lists", "[cli][config]") {
    constexpr array<string_view, 6> expectedParams{
        "datasetPath",
        "resultsNamespace",
        "hashFunction",
        "k",
        "l",
        "lLog"
    };
    constexpr array<string_view, 4> expectedHashes{
        "splitmix64",
        "xxhash64",
        "murmurhash3",
        "siphash24"
    };

    REQUIRE(satp::cli::config::configurableParamNames() == expectedParams);
    REQUIRE(satp::cli::config::supportedHashFunctionNames() == expectedHashes);
}

TEST_CASE("Executor selection supports specific ids and all keyword", "[cli][executor]") {
    {
        const auto selected = satp::cli::executor::collectRequestedAlgorithms({
            "hll",
            "hll",
            "pc"
        });
        REQUIRE(selected.size() == 2u);
        REQUIRE(satp::cli::executor::shouldRun(selected, "hll"));
        REQUIRE(satp::cli::executor::shouldRun(selected, "pc"));
        REQUIRE_FALSE(satp::cli::executor::shouldRun(selected, "ll"));
    }

    {
        const auto selected = satp::cli::executor::collectRequestedAlgorithms({"all"});
        REQUIRE(satp::cli::executor::shouldRun(selected, "hllpp"));
        REQUIRE(satp::cli::executor::shouldRun(selected, "ll"));
        REQUIRE(satp::cli::executor::shouldRun(selected, "pc"));
    }
}

TEST_CASE("Executor output helpers expose stable labels and metrics", "[cli][executor]") {
    using satp::cli::RunMode;
    using satp::cli::executor::algorithmLogPrefix;
    using satp::cli::executor::modeLabel;
    using satp::cli::executor::rseHll;
    using satp::cli::executor::rseLogLog;
    using satp::cli::executor::rseUnknown;

    REQUIRE(string(modeLabel(RunMode::Streaming)) == "streaming");
    REQUIRE(string(modeLabel(RunMode::Merge)) == "merge");

    REQUIRE(abs(rseHll(10u) - 0.0325) < 1e-12);
    REQUIRE(abs(rseLogLog(10u) - 0.040625) < 1e-12);
    REQUIRE(isnan(rseUnknown()));

    const satp::cli::AlgorithmRunSpec hllppSpec{
        "hllpp",
        "k=16",
        "splitmix64",
        0.0
    };
    REQUIRE(algorithmLogPrefix(hllppSpec) == "[hllpp|HyperLogLog++]");

    const satp::cli::AlgorithmRunSpec unknownSpec{
        "not-supported",
        "",
        "splitmix64",
        0.0
    };
    REQUIRE_THROWS_AS(algorithmLogPrefix(unknownSpec), invalid_argument);
}
