// The game's random numbers: reproducible from a seed, never stuck at zero,
// and below(n) without modulo bias.

#include <gtest/gtest.h>

#include "Rng.hpp"

using namespace TrailQuest;

TEST(Rng, TheSameSeedGivesTheSameSequence)
{
    Rng a(12345), b(12345);
    for (int i = 0; i < 1000; ++i) {
        ASSERT_EQ(a.next(), b.next()) << i;
    }
}

TEST(Rng, DifferentSeedsGiveDifferentSequences)
{
    Rng a(1), b(2);
    int same = 0;
    for (int i = 0; i < 100; ++i) {
        same += a.next() == b.next() ? 1 : 0;
    }
    EXPECT_LT(same, 3);
}

TEST(Rng, AZeroSeedIsReplacedSoTheSequenceNeverSticks)
{
    Rng r(0);
    EXPECT_NE(r.state(), 0u);
    EXPECT_NE(r.next(), 0u);
    r.setState(0);
    EXPECT_NE(r.state(), 0u);
}

TEST(Rng, StateRoundTripsThroughASave)
{
    Rng a(777);
    a.next();
    a.next();
    Rng b;
    b.setState(a.state());
    for (int i = 0; i < 50; ++i) {
        ASSERT_EQ(a.next(), b.next());
    }
}

TEST(Rng, BelowStaysInRange)
{
    Rng r(9);
    EXPECT_EQ(r.below(0), 0u);
    EXPECT_EQ(r.below(1), 0u);
    for (uint32_t n : {2u, 3u, 7u, 100u, 1000u, 0x80000001u}) {
        for (int i = 0; i < 2000; ++i) {
            ASSERT_LT(r.below(n), n) << n;
        }
    }
}

TEST(Rng, BelowIsEvenAcrossItsValues)
{
    // 60,000 draws of six values: each should be near 10,000. A biased
    // generator, or a modulo of a short range, would be well outside 5 %.
    Rng r(4242);
    uint32_t counts[6] = {};
    for (int i = 0; i < 60000; ++i) {
        ++counts[r.below(6)];
    }
    for (uint32_t c : counts) {
        EXPECT_GT(c, 9500u);
        EXPECT_LT(c, 10500u);
    }
}

TEST(Rng, ChanceMatchesItsPercentage)
{
    Rng r(31);
    int hits = 0;
    for (int i = 0; i < 100000; ++i) {
        hits += r.chance(30) ? 1 : 0;
    }
    EXPECT_GT(hits, 29000);
    EXPECT_LT(hits, 31000);
    EXPECT_FALSE(r.chance(0));
    EXPECT_TRUE(r.chance(100));
    EXPECT_TRUE(r.chance(250));
}

TEST(Rng, MixSeedSpreadsNeighbouringValues)
{
    EXPECT_NE(mixSeed(1), mixSeed(2));
    EXPECT_NE(mixSeed(20261017), mixSeed(20261018));
}
