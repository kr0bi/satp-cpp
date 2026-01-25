#include "catch2/catch_test_macros.hpp"
#include "TestData.h"

TEST_CASE("Caricamento dataset da file", "[dataset]") {
    auto dataset = satp::testdata::loadDataset();
    REQUIRE(dataset.total == 10000u);
    REQUIRE(dataset.distinct == 1000u);
    REQUIRE(dataset.values.size() == dataset.total);
}
