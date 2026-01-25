#include "catch2/catch_test_macros.hpp"
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
