// include/satp/AlgorithmConcept.hpp
#pragma once
#include <concepts>

#include "../algorithms/Algorithm.h"
#include <concepts>
#include <iostream>
#include <vector>

#include "Loop.h"
#include "satp/Utils.h"

namespace satp {

    /** Qualunque tipo derivato da satp::algorithms::Algorithm. */
    template<typename T>
    concept AlgorithmLike = std::derived_from<T, algorithms::Algorithm>;

}  // namespace satp

namespace satp::simulation {
/**
 * Esegue più prove dell’algoritmo A e calcola media, bias e varianza
 * della stima della cardinalità.
 *
 * @tparam A  qualunque classe che implementi satp::algorithms::Algorithm
 */



template<AlgorithmLike A>
void evaluateAlgorithm(std::size_t trials,
                       std::size_t numberOfElements,
                       std::size_t numberOfDistinctElements)
{
    std::vector<double> estimates;
    estimates.reserve(trials);

    for (std::size_t i = 0; i < trials; ++i) {
        // Stream casuale di IDs con solo «numberOfDistinctElements» diversi
        auto dataStream = utils::getRandomNumbers(numberOfElements,
                                                  numberOfDistinctElements);

        // Nuova istanza dell’algoritmo per ogni trial (come in Java)
        A algo;
        Loop<A> loop(std::move(algo), std::move(dataStream));

        estimates.push_back(static_cast<double>(loop.process()));
    }

    // --- Statistiche -------------------------------------------------------
    double mean = std::accumulate(estimates.begin(), estimates.end(), 0.0) /
                  static_cast<double>(trials);

    double variance = 0.0;
    for (double e : estimates) variance += (e - mean) * (e - mean);
    variance /= static_cast<double>(trials);

    double bias = mean - static_cast<double>(numberOfDistinctElements);

    // --- Output ------------------------------------------------------------
    std::cout << "Trials:             " << trials                   << '\n'
              << "True unique count:  " << numberOfDistinctElements << '\n'
              << "Mean estimate:      " << mean                     << '\n'
              << "Bias:               " << bias                     << '\n'
              << "Variance:           " << variance                 << "\n\n";
}

} // namespace satp::simulation
