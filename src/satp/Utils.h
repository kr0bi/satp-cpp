#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <cassert>
#include <unordered_set>

using namespace std;

namespace satp::utils {
    /**
     * 
     * @param numberOfElements
     * @param to fino a che numero devono essere generati i numeri
     * @return un array di interi generati randomicamente tra 0 e m
     */
    inline vector<uint32_t>
    getRandomNumbers(size_t numberOfElements, int to) {
        assert(to >= 0 && "l'intervallo deve essere non‑negativo");

        // engine: Mersenne Twister 32bit, seedato una sola volta per thread
        static thread_local mt19937 rng{random_device{}()};

        // distribuzione uniforme chiusa [0 , to]
        uniform_int_distribution<uint32_t> dist(
            0u, static_cast<uint32_t>(to));

        vector<uint32_t> out(numberOfElements);
        ranges::generate(out, [&] { return dist(rng); });
        return out;
    }

    inline size_t count_distinct(const vector<uint32_t> &v) {
        return unordered_set<int>(v.begin(), v.end()).size();
    }
} // namespace satp::utils
