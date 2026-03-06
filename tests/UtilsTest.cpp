#include "catch2/catch_test_macros.hpp"
#include "TestData.h"
#include "satp/io/BinaryDatasetIO.h"

#include <unordered_set>

using namespace std;

TEST_CASE("Caricamento dataset da file", "[dataset]") {
    auto dataset = satp::testdata::loadDataset();
    REQUIRE(dataset.elements_per_partition == 2000u);
    REQUIRE(dataset.distinct == 1000u);
    REQUIRE(dataset.partition_count == 3u);
    REQUIRE(dataset.values.size() == dataset.elements_per_partition);

    const auto index = satp::io::indexBinaryDataset(satp::testdata::datasetPath());
    vector<uint8_t> truthBits;
    satp::io::loadBinaryPartitionTruthBits(index, 0, truthBits);
    REQUIRE(truthBits.size() == (dataset.elements_per_partition + 7u) / 8u);

    unordered_set<uint32_t> seen;
    size_t prefixF0 = 0;
    for (size_t i = 0; i < dataset.values.size(); ++i) {
        const auto value = dataset.values[i];
        const bool isNewExpected = seen.insert(value).second;
        const uint8_t byte = truthBits[i >> 3u];
        const bool isNewFromFile = ((byte >> (i & 7u)) & 0x1u) != 0;
        REQUIRE(isNewFromFile == isNewExpected);
        if (isNewFromFile) {
            ++prefixF0;
        }
    }
    REQUIRE(prefixF0 == dataset.distinct);
}
