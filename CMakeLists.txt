cmake_minimum_required(VERSION 3.31)
project(satp LANGUAGES CXX)

# C++23 standard e warning utili
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
find_package(ZLIB REQUIRED)

# ---- Libreria core -------------------------------------------------
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/satp/*.cpp)
add_library(satp STATIC ${SRC_FILES})
target_include_directories(satp PUBLIC src)
target_link_libraries(satp PUBLIC ZLIB::ZLIB)

# --------------------------------------------------------------------
#  ESEGUIBILE **BENCHMARK**  ← NUOVO
# --------------------------------------------------------------------
add_executable(main
        main.cpp
)
target_link_libraries(main
        PRIVATE satp                  #  basta la libreria core
)

# ---- Catch2 via FetchContent --------------------------------------
include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2           # oppure la versione più recente
)
FetchContent_MakeAvailable(Catch2)

# ---- Eseguibile di test -------------------------------------------
file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(satp_tests ${TEST_FILES})
target_link_libraries(satp_tests
        PRIVATE satp
        Catch2::Catch2WithMain)
target_include_directories(satp_tests PRIVATE tests)

# Registra automaticamente i test con CTest
include(CTest)
include(Catch)         # fornito da Catch2
catch_discover_tests(satp_tests)
