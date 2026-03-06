#include "catch2/catch_test_macros.hpp"
#include <cmath>
#include <stdexcept>
#include "satp/hashing/HashFactory.h"
#include "satp/algorithms/LogLog.h"
#include "satp/simulation/Loop.h"
#include "TestData.h"

using namespace std;

namespace {
    const satp::hashing::HashFunction &defaultHash() {
        static const auto hash = satp::hashing::getHashFunctionBy();
        return *hash;
    }
}

TEST_CASE("LogLog stima ~1000 distinti su 10000 campioni", "[log-count]") {
    constexpr uint32_t L = 32;
    constexpr uint32_t K = 10;

    auto dataset = satp::testdata::loadDataset();
    auto NUMBER_OF_UNIQUE_ELEMENTS = dataset.distinct;

    satp::algorithms::LogLog loglog(K, L, defaultHash());
    satp::simulation::Loop loop(move(loglog), move(dataset.values));

    auto estimate = loop.process();

    WARN("Stima = " << estimate);
    WARN("Elementi = " << NUMBER_OF_UNIQUE_ELEMENTS);

    const double m = double(1u << K);
    const double RSE = 1.30 / sqrt(m);
    WARN("RSE = " << RSE);

    REQUIRE(estimate >= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 - 4 * RSE));
    REQUIRE(estimate <= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 + 4 * RSE));
}

TEST_CASE("LogLog valida parametri", "[loglog-params]") {
    REQUIRE_THROWS_AS(satp::algorithms::LogLog(0, 32, defaultHash()), invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::LogLog(3, 32, defaultHash()), invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::LogLog(17, 32, defaultHash()), invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::LogLog(5, 31, defaultHash()), invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::LogLog(5, 33, defaultHash()), invalid_argument);
    REQUIRE_NOTHROW(satp::algorithms::LogLog(4, 32, defaultHash()));
    REQUIRE_NOTHROW(satp::algorithms::LogLog(16, 32, defaultHash()));
}

TEST_CASE("LogLog merge: seriale, commutativita', idempotenza", "[loglog][merge]") {
    constexpr uint32_t K = 10;
    constexpr uint32_t L = 32;
    const auto partA = satp::testdata::loadPartition(0);
    const auto partB = satp::testdata::loadPartition(1);

    satp::algorithms::LogLog a(K, L, defaultHash());
    satp::algorithms::LogLog b(K, L, defaultHash());
    satp::algorithms::LogLog serial(K, L, defaultHash());
    for (const auto v : partA) {
        a.process(v);
        serial.process(v);
    }
    for (const auto v : partB) {
        b.process(v);
        serial.process(v);
    }

    satp::algorithms::LogLog merged = a;
    merged.merge(b);
    REQUIRE(merged.count() == serial.count());

    satp::algorithms::LogLog mergedRev = b;
    mergedRev.merge(a);
    REQUIRE(mergedRev.count() == merged.count());

    satp::algorithms::LogLog idem = a;
    idem.merge(a);
    REQUIRE(idem.count() == a.count());
}

TEST_CASE("LogLog merge valida compatibilita' parametri", "[loglog][merge][params]") {
    satp::algorithms::LogLog a(10, 32, defaultHash());
    satp::algorithms::LogLog bK(11, 32, defaultHash());
    satp::algorithms::LogLog bL(10, 32, defaultHash());
    REQUIRE_THROWS_AS(a.merge(bK), invalid_argument);
    REQUIRE_NOTHROW(a.merge(bL));
}
