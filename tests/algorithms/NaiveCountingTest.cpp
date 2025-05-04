#include <catch2/catch_test_macros.hpp>

#include "satp/Utils.h"
#include "satp/algorithms/NaiveCounting.h"
#include "satp/simulation/Loop.h"


TEST_CASE("NaiveCounting conta 1000 distinti su 10000 campioni", "[naive]") {
    constexpr std::size_t NUMBER_OF_DISTINCT_ELEMENTS = 1000;
    constexpr std::size_t NUMBER_OF_ELEMENTS          = 10000;

    // Genera il vettore di input (10000 elementi in [0,999])
    auto randomInts = satp::utils::getRandomNumbers(NUMBER_OF_ELEMENTS,
                                                    NUMBER_OF_DISTINCT_ELEMENTS);

    satp::algorithms::NaiveCounting algo;
    satp::simulation::Loop loop(std::move(algo), std::move(randomInts));

    auto result = loop.process();

    REQUIRE(result == NUMBER_OF_DISTINCT_ELEMENTS);
}
