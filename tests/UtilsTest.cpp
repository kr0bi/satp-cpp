#include "catch2/catch_test_macros.hpp"
#include "satp/Utils.h"

TEST_CASE("Test delle utility", "[utility]") {
    SECTION("Numero di elementi randomici") {
        auto numeroDiInteri = 10000;
        REQUIRE(satp::utils::getRandomNumbers(numeroDiInteri, 10000).size() == numeroDiInteri);
    }
}


TEST_CASE("Test sul numero di elementi distinti", "[distinct]") {
    SECTION("Contare il numero di elementi distinti") {
        vector<uint32_t> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        REQUIRE(satp::utils::count_distinct(v) == v.size());
    }
    SECTION("Contare il numero di elementi distinti randomici") {
        auto numeroDiInteri = 10;
        REQUIRE(satp::utils::count_distinct(satp::utils::getRandomNumbers(numeroDiInteri, 1)) == numeroDiInteri);
    }
}
