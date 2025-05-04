#pragma once
#include <vector>
#include <random>
#include <cstdint>

namespace satp::utils {

    /**
     * Restituisce un vettore di `numberOfElements` interi pseudo‑casuali,
     * ciascuno nell’intervallo [0, numberOfUniqueElements - 1].
     */
    inline std::vector<std::uint32_t>
    getRandomNumbers(std::size_t numberOfElements,
                     std::size_t numberOfUniqueElements)
    {
        std::vector<std::uint32_t> v;
        v.reserve(numberOfElements);

        std::random_device rd;                       // seed
        std::mt19937 gen(rd());                      // Mersenne Twister 32‑bit
        std::uniform_int_distribution<std::uint32_t>
            dist(0u, static_cast<std::uint32_t>(numberOfUniqueElements - 1));

        for (std::size_t i = 0; i < numberOfElements; ++i)
            v.push_back(dist(gen));

        return v;
    }

} // namespace satp::utils
