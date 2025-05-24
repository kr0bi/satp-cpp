#include <iostream>

#include "satp/io/FileNaming.h"
#include "src/satp/Utils.h"
#include "src/satp/algorithms/HyperLogLog.h"
#include "src/satp/algorithms/HyperLogLogPlusPlus.h"
#include "src/satp/algorithms/LogLog.h"
#include "src/satp/algorithms/ProbabilisticCounting.h"
#include "src/satp/simulation/EvaluationFramework.h"

namespace eval = satp::evaluation;
namespace alg = satp::algorithms;
namespace util = satp::utils;

int main() {
        // ----- parametri del benchmark -------------------------
        constexpr std::size_t HIGHEST_NUMBER = 1'000'000'000;
        constexpr std::size_t NUMBER_OF_ELEMS = 100'000'000;
        constexpr std::size_t SAMPLE_SIZE = 1'000'000;
        constexpr std::size_t RUNS = 1;

        constexpr std::uint32_t K = 16; // registri per HLL/LL
        constexpr std::uint32_t L = 16; // bitmap PC
        constexpr std::uint32_t L_LOG = 32; // bitmap LL

        // genera il nome file
        const std::string cacheFile =
                        satp::io::makeDatasetFilename(NUMBER_OF_ELEMS,
                                                      HIGHEST_NUMBER,
                                                      SAMPLE_SIZE,
                                                      RUNS);

        satp::evaluation::EvaluationFramework bench(
                cacheFile,
                RUNS,
                SAMPLE_SIZE,
                NUMBER_OF_ELEMS,
                HIGHEST_NUMBER);

        // ----- HyperLogLog++ -------------------------------------
        auto hllpp = bench.evaluate<alg::HyperLogLogPlusPlus>(RUNS, SAMPLE_SIZE, K);
        std::cout << "[HLL++] mean=" << hllpp.mean
                        << "  diff=" << hllpp.difference
                        << "  var=" << hllpp.variance
                        << "  bias=" << hllpp.bias << '\n';

        // ----- HyperLogLog -------------------------------------
        auto hll = bench.evaluate<alg::HyperLogLog>(RUNS, SAMPLE_SIZE, K, L_LOG);
        std::cout << "[HLL] mean=" << hll.mean
                        << "  diff=" << hll.difference
                        << "  var=" << hll.variance
                        << "  bias=" << hll.bias << '\n';

        // ----- LogLog ------------------------------------------
        auto ll = bench.evaluate<alg::LogLog>(RUNS, SAMPLE_SIZE, K, L_LOG);
        std::cout << "[LL ] mean=" << ll.mean
                        << "  diff=" << ll.difference
                        << "  var=" << ll.variance
                        << "  bias=" << ll.bias << '\n';

        // ----- ProbabilisticCounting ---------------------------
        auto pc = bench.evaluate<alg::ProbabilisticCounting>(RUNS, SAMPLE_SIZE, L);
        std::cout << "[PC ] mean=" << pc.mean
                        << "  diff=" << pc.difference
                        << "  var=" << pc.variance
                        << "  bias=" << pc.bias << '\n';
}
