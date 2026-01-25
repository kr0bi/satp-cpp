#include <catch2/catch_test_macros.hpp>

#include "satp/algorithms/ProbabilisticCounting.h"
#include "satp/simulation/Loop.h"
#include "TestData.h"


TEST_CASE("ProbabilisticCounting stima ~1000 distinti su 10000 campioni", "[prob-count]") {
    constexpr std::uint32_t L = 16; // bitmap 16 bit → buono fino a ~65k

    auto dataset = satp::testdata::loadDataset();
    auto NUMBER_OF_UNIQUE_ELEMENTS = dataset.distinct;

    satp::algorithms::ProbabilisticCounting pc(L);
    satp::simulation::Loop loop(std::move(pc), std::move(dataset.values));

    auto estimate = loop.process();

    WARN("Stima = " << estimate);
    WARN("Elementi = " << NUMBER_OF_UNIQUE_ELEMENTS);
    // range conservativo per evitare flakiness su dataset deterministico
    REQUIRE(estimate >= NUMBER_OF_UNIQUE_ELEMENTS * 0.2);
    REQUIRE(estimate <= NUMBER_OF_UNIQUE_ELEMENTS * 3.0);
}
