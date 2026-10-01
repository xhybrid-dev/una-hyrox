// lapSeconds: laps must start where the last one ended and sum to the session.

#include <gtest/gtest.h>

#include <cstdint>

#include "LapTiming.hpp"

TEST(LapTiming, LapsSumToTheSessionNotTheRoundedDownParts)
{
    // The 31 laps of the 1 October test, with sub-second parts typical of a
    // button press: each lap's own seconds, rounded down, lost ~0.5 s.
    const uint32_t lapMs[] = {
        95700, 14600, 234700, 21400, 89500, 38800, 95200, 16600,
        97100, 65300, 217900, 12700, 102300, 45200, 251900, 25800,
        102600, 27100, 239200, 24400, 109300, 16900, 171600, 21200,
        112800, 33400, 248600, 23100, 106400, 21900, 264700};
    constexpr size_t n = sizeof(lapMs) / sizeof(lapMs[0]);

    uint32_t cursor = 0;
    uint32_t sumElapsed = 0;
    uint32_t sumNaive = 0;
    uint32_t expectedStart = 0;
    for (size_t i = 0; i < n; ++i) {
        const Race::LapSeconds l = Race::lapSeconds(cursor, lapMs[i], 0);
        EXPECT_EQ(l.startSec, expectedStart) << "lap " << i << " must start where the last ended";
        EXPECT_EQ(l.activeSec, l.elapsedSec);
        expectedStart += l.elapsedSec;
        sumElapsed += l.elapsedSec;
        sumNaive += lapMs[i] / 1000u;
        cursor += lapMs[i];
    }
    EXPECT_EQ(sumElapsed, cursor / 1000u);
    EXPECT_LT(sumNaive, sumElapsed) << "the old rounding really did lose time";
}

TEST(LapTiming, PausedTimeComesOffTheTimerButNotTheElapsed)
{
    const Race::LapSeconds l = Race::lapSeconds(0u, 60000u, 10000u);
    EXPECT_EQ(l.elapsedSec, 70u);
    EXPECT_EQ(l.activeSec, 60u);
}

TEST(LapTiming, NeverUnderflows)
{
    const Race::LapSeconds l = Race::lapSeconds(0u, 0u, 600u);
    EXPECT_EQ(l.elapsedSec, 0u);
    EXPECT_EQ(l.activeSec, 0u);
}
