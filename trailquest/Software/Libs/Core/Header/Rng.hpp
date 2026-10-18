/**
 ******************************************************************************
 * @file    Rng.hpp
 * @brief   The game's one source of chance: a seeded xorshift32.
 *
 * Every random choice in the game goes through this, and its whole state is one
 * 32-bit word that the save file keeps. So a seed and the player's inputs
 * always replay the same game, on the host tests and on the watch alike. Never
 * std::rand (its sequence differs by library) and never floating point.
 *
 * Header-only and tiny.
 ******************************************************************************
 */

#ifndef TRAILQUEST_RNG_HPP
#define TRAILQUEST_RNG_HPP

#include <cstdint>

namespace TrailQuest
{

class Rng
{
public:
    /// A zero state would stay zero for ever, so it is replaced.
    explicit Rng(uint32_t seed = 1u) : mState(seed != 0u ? seed : kZeroSeed) {}

    uint32_t state() const { return mState; }
    void     setState(uint32_t s) { mState = s != 0u ? s : kZeroSeed; }

    /// The next 32 random bits (Marsaglia's xorshift32, 13/17/5).
    uint32_t next()
    {
        uint32_t x = mState;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        mState = x;
        return x;
    }

    /// A number in [0, n), every value equally likely (rejection, not a bare
    /// modulo, which would favour the low values). 0 for n == 0.
    uint32_t below(uint32_t n)
    {
        if (n == 0u) {
            return 0u;
        }
        // The largest multiple of n that fits; draws at or above it are thrown
        // away. (0u - n) % n == 2^32 mod n.
        const uint32_t reject = (0u - n) % n;
        uint32_t r;
        do {
            r = next();
        } while (r < reject);
        return r % n;
    }

    /// True with probability @p percent / 100 (0 never, 100 or more always).
    bool chance(uint32_t percent)
    {
        if (percent >= 100u) {
            return true;
        }
        return below(100u) < percent;
    }

private:
    static constexpr uint32_t kZeroSeed = 0x6D2B79F5u;

    uint32_t mState;
};

/// Mix a value into a well-spread 32-bit seed (the finaliser of splitmix32).
inline uint32_t mixSeed(uint32_t x)
{
    x += 0x9E3779B9u;
    x = (x ^ (x >> 16)) * 0x85EBCA6Bu;
    x = (x ^ (x >> 13)) * 0xC2B2AE35u;
    return x ^ (x >> 16);
}

} // namespace TrailQuest

#endif // TRAILQUEST_RNG_HPP
