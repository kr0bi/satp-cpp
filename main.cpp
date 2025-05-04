

#include "src/satp/algorithms/Algorithm.h"
#include "src/satp/algorithms/NaiveCounting.h"
#include "src/satp/simulation/EvaluationFramework.h"

#include "src/satp/simulation/Loop.h"

int main()
{
    constexpr std::size_t TRIALS      = 100;
    constexpr std::size_t ELEMENTS    = 10000;
    constexpr std::size_t DISTINCT    = 1000;

    satp::simulation::evaluateAlgorithm<satp::algorithms::NaiveCounting>(TRIALS, ELEMENTS, DISTINCT);

    return 0;
}
