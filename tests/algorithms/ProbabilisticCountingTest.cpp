#include <catch2/catch_test_macros.hpp>

#include "satp/Utils.h"
#include "satp/algorithms/ProbabilisticCounting.h"
#include "satp/simulation/Loop.h"


TEST_CASE("ProbabilisticCounting stima ~1000 distinti su 10000 campioni", "[prob-count]") {
    constexpr std::size_t HIGHEST_NUMBER = 100000;
    constexpr std::size_t NUMBER_OF_ELEMENTS = 500000;
    constexpr std::uint32_t L = 16; // bitmap 16 bit → buono fino a ~65k

    auto randomInts = satp::utils::getRandomNumbers(NUMBER_OF_ELEMENTS,
                                                    HIGHEST_NUMBER);
    auto NUMBER_OF_UNIQUE_ELEMENTS = satp::utils::count_distinct(randomInts);

    satp::algorithms::ProbabilisticCounting pc(L);
    satp::simulation::Loop loop(std::move(pc), std::move(randomInts));

    auto estimate = loop.process();

    WARN("Stima = " << estimate);
    WARN("Elementi = " << NUMBER_OF_UNIQUE_ELEMENTS);
    // entro ±16% con confidenza ragionevole
    REQUIRE(estimate >= NUMBER_OF_UNIQUE_ELEMENTS * 0.84);
    REQUIRE(estimate <= NUMBER_OF_UNIQUE_ELEMENTS * 1.1);
}
