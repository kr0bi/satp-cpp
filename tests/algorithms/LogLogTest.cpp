#include "catch2/catch_test_macros.hpp"
#include "satp/Utils.h"
#include "satp/algorithms/LogLog.h"
#include "satp/simulation/Loop.h"


TEST_CASE("LogLog stima ~1000 distinti su 10000 campioni", "[log-count]") {
    constexpr std::size_t HIGHEST_NUMBER = 100'000;
    constexpr std::size_t NUMBER_OF_ELEMENTS = 1'000'000;
    constexpr std::uint32_t L = 32;
    constexpr std::uint32_t K = 10;

    auto randomInts = satp::utils::getRandomNumbers(NUMBER_OF_ELEMENTS,
                                                    HIGHEST_NUMBER);
    auto NUMBER_OF_UNIQUE_ELEMENTS = satp::utils::count_distinct(randomInts);

    satp::algorithms::LogLog loglog(K, L);
    satp::simulation::Loop loop(std::move(loglog), std::move(randomInts));

    auto estimate = loop.process();

    WARN("Stima = " << estimate);
    WARN("Elementi = " << NUMBER_OF_UNIQUE_ELEMENTS);

    auto denom = 1.0 / std::sqrt(double(1u << K));
    const double RSE = 1.0 / denom;
    WARN("RSE = " << 1.0 / denom);

    REQUIRE(estimate >= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 - 3 * RSE));
    REQUIRE(estimate <= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 + 3 * RSE));
}
