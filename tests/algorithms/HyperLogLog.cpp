#include "catch2/catch_test_macros.hpp"
#include "satp/algorithms/HyperLogLog.h"
#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/simulation/Loop.h"
#include "TestData.h"


TEST_CASE("HyperLogLog stima ~1000 distinti su 10000 campioni", "[hyperloglog-count]") {
    constexpr std::uint32_t K = 10;

    auto dataset = satp::testdata::loadDataset();
    auto NUMBER_OF_UNIQUE_ELEMENTS = dataset.distinct;

    satp::algorithms::HyperLogLogPlusPlus loglog(K);
    satp::simulation::Loop loop(std::move(loglog), std::move(dataset.values));

    auto estimate = loop.process();

    WARN("Stima = " << estimate);
    WARN("Elementi = " << NUMBER_OF_UNIQUE_ELEMENTS);

    auto denom = 1.0 / std::sqrt(double(1u << K));
    const double RSE = 1.0 / denom;
    WARN("RSE = " << 1.0 / denom);

    REQUIRE(estimate >= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 - 3 * RSE));
    REQUIRE(estimate <= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 + 3 * RSE));
}
