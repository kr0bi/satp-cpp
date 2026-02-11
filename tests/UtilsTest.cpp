#include "catch2/catch_test_macros.hpp"
#include "TestData.h"

TEST_CASE("Caricamento dataset da file", "[dataset]") {
    auto dataset = satp::testdata::loadDataset();
    REQUIRE(dataset.elements_per_partition == 2000u);
    REQUIRE(dataset.distinct == 1000u);
    REQUIRE(dataset.partition_count == 3u);
    REQUIRE(dataset.values.size() == dataset.elements_per_partition);
}
