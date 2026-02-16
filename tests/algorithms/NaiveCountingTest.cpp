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

TEST_CASE("NaiveCounting merge: seriale, commutativita', idempotenza", "[naive][merge]") {
    const auto partA = satp::testdata::loadPartition(0);
    const auto partB = satp::testdata::loadPartition(1);

    satp::algorithms::NaiveCounting a;
    satp::algorithms::NaiveCounting b;
    satp::algorithms::NaiveCounting serial;
    for (const auto v : partA) {
        a.process(v);
        serial.process(v);
    }
    for (const auto v : partB) {
        b.process(v);
        serial.process(v);
    }

    satp::algorithms::NaiveCounting merged = a;
    merged.merge(b);
    REQUIRE(merged.count() == serial.count());

    satp::algorithms::NaiveCounting mergedRev = b;
    mergedRev.merge(a);
    REQUIRE(mergedRev.count() == merged.count());

    satp::algorithms::NaiveCounting idem = a;
    idem.merge(a);
    REQUIRE(idem.count() == a.count());
}
