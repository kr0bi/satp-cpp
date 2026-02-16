#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

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

TEST_CASE("ProbabilisticCounting merge: seriale, commutativita', idempotenza", "[prob-count][merge]") {
    constexpr std::uint32_t L = 16;
    const auto partA = satp::testdata::loadPartition(0);
    const auto partB = satp::testdata::loadPartition(1);

    satp::algorithms::ProbabilisticCounting a(L);
    satp::algorithms::ProbabilisticCounting b(L);
    satp::algorithms::ProbabilisticCounting serial(L);
    for (const auto v : partA) {
        a.process(v);
        serial.process(v);
    }
    for (const auto v : partB) {
        b.process(v);
        serial.process(v);
    }

    satp::algorithms::ProbabilisticCounting merged(L);
    merged = a;
    merged.merge(b);
    REQUIRE(merged.count() == serial.count());

    satp::algorithms::ProbabilisticCounting mergedRev(L);
    mergedRev = b;
    mergedRev.merge(a);
    REQUIRE(mergedRev.count() == merged.count());

    satp::algorithms::ProbabilisticCounting idem(L);
    idem = a;
    idem.merge(a);
    REQUIRE(idem.count() == a.count());
}

TEST_CASE("ProbabilisticCounting merge valida compatibilita' parametri", "[prob-count][merge][params]") {
    satp::algorithms::ProbabilisticCounting a(16);
    satp::algorithms::ProbabilisticCounting b(15);
    REQUIRE_THROWS_AS(a.merge(b), std::invalid_argument);
}
