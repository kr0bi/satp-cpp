#include <catch2/catch_test_macros.hpp>

#include "satp/algorithms/NaiveCounting.h"
#include "satp/simulation/Loop.h"
#include "TestData.h"


TEST_CASE("NaiveCounting conta 1000 distinti su 10000 campioni", "[naive]") {
    auto dataset = satp::testdata::loadDataset();
    auto NUMBER_OF_UNIQUE_ELEMENTS = dataset.distinct;

    satp::algorithms::NaiveCounting algo;
    satp::simulation::Loop loop(std::move(algo), std::move(dataset.values));

    auto result = loop.process();

    REQUIRE(result == NUMBER_OF_UNIQUE_ELEMENTS);
}
