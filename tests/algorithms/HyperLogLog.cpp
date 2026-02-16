#include "catch2/catch_test_macros.hpp"
#include <stdexcept>
#include "satp/algorithms/HyperLogLog.h"
#include "satp/algorithms/HyperLogLogPlusPlus.h"
#include "satp/simulation/Loop.h"
#include "TestData.h"


TEST_CASE("HyperLogLog stima ~1000 distinti su 10000 campioni", "[hyperloglog-count]") {
    constexpr std::uint32_t K = 10;

    auto dataset = satp::testdata::loadDataset();
    auto NUMBER_OF_UNIQUE_ELEMENTS = dataset.distinct;

    satp::algorithms::HyperLogLog hll(K, 32);
    satp::simulation::Loop loop(std::move(hll), std::move(dataset.values));

    auto estimate = loop.process();

    WARN("Stima = " << estimate);
    WARN("Elementi = " << NUMBER_OF_UNIQUE_ELEMENTS);

    const double m = double(1u << K);
    const double RSE = 1.04 / std::sqrt(m);
    WARN("RSE = " << RSE);

    REQUIRE(estimate >= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 - 3 * RSE));
    REQUIRE(estimate <= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 + 3 * RSE));
}

TEST_CASE("HyperLogLog++ stima ~1000 distinti su 10000 campioni", "[hyperloglogpp-count]") {
    constexpr std::uint32_t K = 10;

    auto dataset = satp::testdata::loadDataset();
    auto NUMBER_OF_UNIQUE_ELEMENTS = dataset.distinct;

    satp::algorithms::HyperLogLogPlusPlus hllpp(K);
    satp::simulation::Loop loop(std::move(hllpp), std::move(dataset.values));

    auto estimate = loop.process();

    WARN("Stima = " << estimate);
    WARN("Elementi = " << NUMBER_OF_UNIQUE_ELEMENTS);

    const double m = double(1u << K);
    const double RSE = 1.04 / std::sqrt(m);
    WARN("RSE = " << RSE);

    REQUIRE(estimate >= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 - 3 * RSE));
    REQUIRE(estimate <= NUMBER_OF_UNIQUE_ELEMENTS * (1.0 + 3 * RSE));
}

TEST_CASE("HyperLogLog valida parametri", "[hyperloglog-params]") {
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLog(0, 32), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLog(3, 32), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLog(17, 32), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLog(5, 31), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLog(5, 33), std::invalid_argument);
    REQUIRE_NOTHROW(satp::algorithms::HyperLogLog(4, 32));
    REQUIRE_NOTHROW(satp::algorithms::HyperLogLog(16, 32));
}

TEST_CASE("HyperLogLog++ valida parametri", "[hyperloglogpp-params]") {
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLogPlusPlus(0), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLogPlusPlus(3), std::invalid_argument);
    REQUIRE_THROWS_AS(satp::algorithms::HyperLogLogPlusPlus(19), std::invalid_argument);
}

TEST_CASE("HyperLogLog++ supporta i limiti p=4 e p=18", "[hyperloglogpp-params]") {
    auto dataset = satp::testdata::loadDataset();

    satp::algorithms::HyperLogLogPlusPlus hllppMin(4);
    satp::simulation::Loop loopMin(std::move(hllppMin), dataset.values);
    const auto estimateMin = loopMin.process();
    REQUIRE(estimateMin > 0);

    satp::algorithms::HyperLogLogPlusPlus hllppMax(18);
    satp::simulation::Loop loopMax(std::move(hllppMax), dataset.values);
    const auto estimateMax = loopMax.process();
    REQUIRE(estimateMax > 0);
}

TEST_CASE("HyperLogLog merge: seriale, commutativita', idempotenza", "[hyperloglog][merge]") {
    constexpr std::uint32_t K = 10;
    constexpr std::uint32_t L = 32;
    const auto partA = satp::testdata::loadPartition(0);
    const auto partB = satp::testdata::loadPartition(1);

    satp::algorithms::HyperLogLog a(K, L);
    satp::algorithms::HyperLogLog b(K, L);
    satp::algorithms::HyperLogLog serial(K, L);
    for (const auto v : partA) {
        a.process(v);
        serial.process(v);
    }
    for (const auto v : partB) {
        b.process(v);
        serial.process(v);
    }

    satp::algorithms::HyperLogLog merged = a;
    merged.merge(b);
    REQUIRE(merged.count() == serial.count());

    satp::algorithms::HyperLogLog mergedRev = b;
    mergedRev.merge(a);
    REQUIRE(mergedRev.count() == merged.count());

    satp::algorithms::HyperLogLog idem = a;
    idem.merge(a);
    REQUIRE(idem.count() == a.count());
}

TEST_CASE("HyperLogLog merge valida compatibilita' parametri", "[hyperloglog][merge][params]") {
    satp::algorithms::HyperLogLog a(10, 32);
    satp::algorithms::HyperLogLog b(11, 32);
    REQUIRE_THROWS_AS(a.merge(b), std::invalid_argument);
}

TEST_CASE("HyperLogLog++ merge: seriale, commutativita', idempotenza", "[hyperloglogpp][merge]") {
    constexpr std::uint32_t P = 10;
    const auto partA = satp::testdata::loadPartition(0);
    const auto partB = satp::testdata::loadPartition(1);

    satp::algorithms::HyperLogLogPlusPlus a(P);
    satp::algorithms::HyperLogLogPlusPlus b(P);
    satp::algorithms::HyperLogLogPlusPlus serial(P);
    for (const auto v : partA) {
        a.process(v);
        serial.process(v);
    }
    for (const auto v : partB) {
        b.process(v);
        serial.process(v);
    }

    satp::algorithms::HyperLogLogPlusPlus merged = a;
    merged.merge(b);
    const double serialEstimate = static_cast<double>(serial.count());
    const double mergedEstimate = static_cast<double>(merged.count());
    const double relDelta = (serialEstimate != 0.0)
                                ? std::abs(mergedEstimate - serialEstimate) / serialEstimate
                                : 0.0;
    REQUIRE(relDelta <= 0.05);

    satp::algorithms::HyperLogLogPlusPlus mergedRev = b;
    mergedRev.merge(a);
    REQUIRE(mergedRev.count() == merged.count());

    satp::algorithms::HyperLogLogPlusPlus idem = a;
    idem.merge(a);
    const double aEstimate = static_cast<double>(a.count());
    const double idemEstimate = static_cast<double>(idem.count());
    const double idemRelDelta = (aEstimate != 0.0)
                                    ? std::abs(idemEstimate - aEstimate) / aEstimate
                                    : 0.0;
    REQUIRE(idemRelDelta <= 0.05);
}

TEST_CASE("HyperLogLog++ merge valida compatibilita' parametri", "[hyperloglogpp][merge][params]") {
    satp::algorithms::HyperLogLogPlusPlus a(10);
    satp::algorithms::HyperLogLogPlusPlus b(11);
    REQUIRE_THROWS_AS(a.merge(b), std::invalid_argument);
}

TEST_CASE("HyperLogLog++ merge sparse+normal equivale al seriale", "[hyperloglogpp][merge][format]") {
    constexpr std::uint32_t P = 10;
    satp::algorithms::HyperLogLogPlusPlus sparse(P);
    satp::algorithms::HyperLogLogPlusPlus normal(P);
    satp::algorithms::HyperLogLogPlusPlus serial(P);

    // Parte piccola: resta in sparse.
    for (std::uint32_t v = 0; v < 128; ++v) {
        sparse.process(v);
        serial.process(v);
    }

    // Parte grande: forza il passaggio a normal (piu' flush del tmpSet).
    for (std::uint32_t v = 10'000; v < 40'000; ++v) {
        normal.process(v);
        serial.process(v);
    }

    satp::algorithms::HyperLogLogPlusPlus merged = sparse;
    merged.merge(normal);
    const double serialEstimate = static_cast<double>(serial.count());
    const double mergedEstimate = static_cast<double>(merged.count());
    const double relDelta = (serialEstimate != 0.0)
                                ? std::abs(mergedEstimate - serialEstimate) / serialEstimate
                                : 0.0;
    REQUIRE(relDelta <= 0.05);
}
