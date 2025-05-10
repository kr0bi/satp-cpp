#include <catch2/catch_test_macros.hpp>

#include "satp/Utils.h"
#include "satp/algorithms/NaiveCounting.h"
#include "satp/simulation/Loop.h"


TEST_CASE("NaiveCounting conta 1000 distinti su 10000 campioni", "[naive]") {
    constexpr std::size_t HIGHEST_NUMBER = 1000;
    constexpr std::size_t NUMBER_OF_ELEMENTS = 10000;

    auto randomInts = satp::utils::getRandomNumbers(NUMBER_OF_ELEMENTS,
                                                    HIGHEST_NUMBER);
    auto NUMBER_OF_UNIQUE_ELEMENTS = satp::utils::count_distinct(randomInts);

    satp::algorithms::NaiveCounting algo;
    satp::simulation::Loop loop(std::move(algo), std::move(randomInts));

    auto result = loop.process();

    REQUIRE(result == NUMBER_OF_UNIQUE_ELEMENTS);
}
