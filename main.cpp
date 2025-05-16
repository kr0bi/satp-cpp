#include <iostream>
#include "src/satp/Utils.h"
#include "src/satp/algorithms/HyperLogLog.h"
#include "src/satp/algorithms/LogLog.h"
#include "src/satp/algorithms/ProbabilisticCounting.h"
#include "src/satp/simulation/EvaluationFramework.h"

namespace eval = satp::evaluation;
namespace alg = satp::algorithms;
namespace util = satp::utils;

int main() {
        // ----- parametri del benchmark -------------------------
        constexpr std::size_t HIGHEST_NUMBER = 1'000'000'000;
        constexpr std::size_t NUMBER_OF_ELEMS = 10'000'000;
        constexpr std::size_t RUNS = 30;

        constexpr std::uint32_t K = 16; // registri per HLL/LL
        constexpr std::uint32_t L = 16; // bitmap PC
        constexpr std::uint32_t L_LOG = 32; // bitmap LL

        // ----- dataset casuale ---------------------------------
        auto data = util::getRandomNumbers(NUMBER_OF_ELEMS, HIGHEST_NUMBER);

        eval::EvaluationFramework bench(std::move(data));

        std::cout << "Ground-truth distinct = " << bench.ground_truth() << '\n';

        // ----- HyperLogLog -------------------------------------
        auto hll = bench.evaluate<alg::HyperLogLog>(RUNS, K, L_LOG);
        std::cout << "[HLL] mean=" << hll.mean
                        << "  diff=" << hll.difference
                        << "  var=" << hll.variance
                        << "  bias=" << hll.bias << '\n';

        // ----- LogLog ------------------------------------------
        auto ll = bench.evaluate<alg::LogLog>(RUNS, K, L_LOG);
        std::cout << "[LL ] mean=" << ll.mean
                        << "  diff=" << ll.difference
                        << "  var=" << ll.variance
                        << "  bias=" << ll.bias << '\n';

        // ----- ProbabilisticCounting ---------------------------
        auto pc = bench.evaluate<alg::ProbabilisticCounting>(RUNS, L);
        std::cout << "[PC ] mean=" << pc.mean
                        << "  diff=" << pc.difference
                        << "  var=" << pc.variance
                        << "  bias=" << pc.bias << '\n';
}
