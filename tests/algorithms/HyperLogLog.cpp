#include "catch2/catch_test_macros.hpp"
#include <stdexcept>
#include "satp/algorithms/HyperLogLog.h"
#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/simulation/Loop.h"
#include "TestData.h"


TEST_CASE("HyperLogLog stima ~1000 distinti su 10000 campioni", "[hyperloglog-count]") {
    constexpr std::uint32_t K = 10;

    auto dataset = satp::testdata::loadDataset();
    auto NUMBER_OF_UNIQUE_ELEMENTS = dataset.distinct;

    satp::algorithms::HyperLogLog hll(K, 32);
    satp::simulation::Loop loop(std::move(hll), std::move(dataset.values));

    auto estimate = loop.process();

    WARN("Stima = " << estimate);
    WARN("Elementi = " << NUMBER_OF_UNIQUE_ELEMENTS);

    const double m = double(1u << K);
    const double RSE = 1.04 / std::sqrt(m);
    WARN("RSE = " << RSE);

    REQUIRE(estimate >= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 - 3 * RSE));
    REQUIRE(estimate <= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 + 3 * RSE));
}

TEST_CASE("HyperLogLog++ stima ~1000 distinti su 10000 campioni", "[hyperloglogpp-count]") {
    constexpr std::uint32_t K = 10;

    auto dataset = satp::testdata::loadDataset();
    auto NUMBER_OF_UNIQUE_ELEMENTS = dataset.distinct;

    satp::algorithms::HyperLogLogPlusPlus hllpp(K);
    satp::simulation::Loop loop(std::move(hllpp), std::move(dataset.values));

    auto estimate = loop.process();

    WARN("Stima = " << estimate);
    WARN("Elementi = " << NUMBER_OF_UNIQUE_ELEMENTS);

    const double m = double(1u << K);
    const double RSE = 1.04 / std::sqrt(m);
    WARN("RSE = " << RSE);

    REQUIRE(estimate >= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 - 3 * RSE));
    REQUIRE(estimate <= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 + 3 * RSE));
}

TEST_CASE("HyperLogLog valida parametri", "[hyperloglog-params]") {
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLog(0, 32), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLog(3, 32), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLog(17, 32), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLog(5, 31), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLog(5, 33), std::invalid_argument);
    REQUIRE_NOTHROW(satp::algorithms::HyperLogLog(4, 32));
    REQUIRE_NOTHROW(satp::algorithms::HyperLogLog(16, 32));
}

TEST_CASE("HyperLogLog++ valida parametri", "[hyperloglogpp-params]") {
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLogPlusPlus(0), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLogPlusPlus(3), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLogPlusPlus(19), std::invalid_argument);
}

TEST_CASE("HyperLogLog++ supporta i limiti p=4 e p=18", "[hyperloglogpp-params]") {
    auto dataset = satp::testdata::loadDataset();

    satp::algorithms::HyperLogLogPlusPlus hllppMin(4);
    satp::simulation::Loop loopMin(std::move(hllppMin), dataset.values);
    const auto estimateMin = loopMin.process();
    REQUIRE(estimateMin > 0);

    satp::algorithms::HyperLogLogPlusPlus hllppMax(18);
    satp::simulation::Loop loopMax(std::move(hllppMax), dataset.values);
    const auto estimateMax = loopMax.process();
    REQUIRE(estimateMax > 0);
}
