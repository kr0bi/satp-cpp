#include <catch2/catch_test_macros.hpp>

#include "satp/Utils.h"
#include "satp/algorithms/ProbabilisticCounting.h"
#include "satp/simulation/Loop.h"


TEST_CASE("ProbabilisticCounting stima ~1000 distinti su 10000 campioni", "[prob-count]") {
    constexpr std::size_t UNIQUE = 100000;
    constexpr std::size_t TOTAL = 10000000;
    constexpr std::uint32_t L = 16; // bitmap 16 bit → buono fino a ~65k

    auto ids = satp::utils::getRandomNumbers(TOTAL, UNIQUE);

    satp::algorithms::ProbabilisticCounting pc(L);
    satp::simulation::Loop loop(pc, ids);

    auto estimate = loop.process();

    INFO("Stima = " << estimate);
    // entro ±16% con confidenza ragionevole
    REQUIRE(estimate >= UNIQUE * 0.84);
    REQUIRE(estimate <= UNIQUE * 1.1);
}
