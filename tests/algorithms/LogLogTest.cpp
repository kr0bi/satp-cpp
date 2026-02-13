#include "catch2/catch_test_macros.hpp"
#include <stdexcept>
#include "satp/algorithms/LogLog.h"
#include "satp/simulation/Loop.h"
#include "TestData.h"


TEST_CASE("LogLog stima ~1000 distinti su 10000 campioni", "[log-count]") {
    constexpr std::uint32_t L = 32;
    constexpr std::uint32_t K = 10;

    auto dataset = satp::testdata::loadDataset();
    auto NUMBER_OF_UNIQUE_ELEMENTS = dataset.distinct;

    satp::algorithms::LogLog loglog(K, L);
    satp::simulation::Loop loop(std::move(loglog), std::move(dataset.values));

    auto estimate = loop.process();

    WARN("Stima = " << estimate);
    WARN("Elementi = " << NUMBER_OF_UNIQUE_ELEMENTS);

    const double m = double(1u << K);
    const double RSE = 1.30 / std::sqrt(m);
    WARN("RSE = " << RSE);

    REQUIRE(estimate >= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 - 4 * RSE));
    REQUIRE(estimate <= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 + 4 * RSE));
}

TEST_CASE("LogLog valida parametri", "[loglog-params]") {
    REQUIRE_THROWS_AS(satp::algorithms::LogLog(0, 32), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::LogLog(3, 32), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::LogLog(17, 32), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::LogLog(5, 31), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::LogLog(5, 33), std::invalid_argument);
    REQUIRE_NOTHROW(satp::algorithms::LogLog(4, 32));
    REQUIRE_NOTHROW(satp::algorithms::LogLog(16, 32));
}
