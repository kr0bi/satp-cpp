#include <array>

#include "catch2/catch_test_macros.hpp"

#include "satp/algorithms/AlgorithmCatalog.h"

using namespace std;

TEST_CASE("Algorithm catalog resolves canonical ids and names", "[algorithms][catalog]") {
    using satp::algorithms::catalog::getIdsOfSupportedAlgorithms;
    using satp::algorithms::catalog::getNameBy;

    REQUIRE(getNameBy("hllpp") == "HyperLogLog++");
    REQUIRE(getNameBy("hll") == "HyperLogLog");
    REQUIRE(getNameBy("ll") == "LogLog");
    REQUIRE(getNameBy("pc") == "Probabilistic Counting");
    REQUIRE(getNameBy("naive") == "Naive");

    constexpr array<string_view, 4> expectedIds{
        "hllpp",
        "hll",
        "ll",
        "pc"
    };
    REQUIRE(getIdsOfSupportedAlgorithms() == expectedIds);

    REQUIRE_THROWS_AS(getNameBy("not-an-algorithm"), invalid_argument);
}
