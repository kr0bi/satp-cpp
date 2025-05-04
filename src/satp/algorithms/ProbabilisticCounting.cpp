#include "ProbabilisticCounting.h"
#include <cmath>
#include <stdexcept>

namespace satp::algorithms {

// ------------------------------------------------------------------
// constexpr MurmurHash3 finaliser (32‑bit)
// ------------------------------------------------------------------
std::uint32_t ProbabilisticCounting::mix32(std::uint32_t h)
{
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

// ------------------------------------------------------------------
// ctor
// ------------------------------------------------------------------
ProbabilisticCounting::ProbabilisticCounting(std::uint32_t L)
    : L_(L)
    , mask_(L == 32 ? std::numeric_limits<std::uint32_t>::max()
                    : (1u << L) - 1u)
    , bitmap_(L, false)
{
    if (L_ == 0 || L_ > 31)
        throw std::invalid_argument("L must be in [1,31]");
}

// ------------------------------------------------------------------
// hashing in [0, 2^L - 1]
// ------------------------------------------------------------------
std::uint32_t ProbabilisticCounting::uniformHash(std::uint32_t id) const
{
    return mix32(id) & mask_;
}

// ------------------------------------------------------------------
// process
// ------------------------------------------------------------------
void ProbabilisticCounting::process(std::uint32_t id)
{
    std::uint32_t h = uniformHash(id);
    if (h == 0) return;                        // ρ == L -> ignora

    std::uint32_t rho = std::countr_one(h);   // 0 … L-1
    if (rho < L_)
        bitmap_[rho] = true;
}

// ------------------------------------------------------------------
// count
// ------------------------------------------------------------------
std::uint64_t ProbabilisticCounting::count() const
{
    std::uint32_t R = nextClearBit(0);         // primo 0 in M
    if (R > L_)  R = L_;                       // bitmap pieno

    double est = std::ldexp(1.0, R) / INV_PHI; // 2^R / φ
    return std::llround(est);
}

void ProbabilisticCounting::reset()
{
    std::fill(bitmap_.begin(), bitmap_.end(), false);
}

// ------------------------------------------------------------------
// helper: primo bit a 0 a partire da 'from'
// ------------------------------------------------------------------
std::uint32_t ProbabilisticCounting::nextClearBit(std::uint32_t from) const
{
    for (std::uint32_t i = from; i < L_; ++i)
        if (!bitmap_[i]) return i;
    return L_;
}

} // namespace satp::algorithms
