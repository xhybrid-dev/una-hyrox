#include <gtest/gtest.h>

#include "SecondGate.hpp"

using Workout::SecondGate;

TEST(SecondGate, FirstCallGoesThrough)
{
    SecondGate g;
    EXPECT_TRUE(g.take(1000));
}

TEST(SecondGate, OnePerSecondHoweverOftenItIsPolled)
{
    SecondGate g;
    int through = 0;
    // 200 ms polling for 60 s: 300 polls, 60 seconds.
    for (int poll = 0; poll < 300; ++poll) {
        const int64_t utc = 5000 + (poll * 200) / 1000;
        through += g.take(utc) ? 1 : 0;
    }
    EXPECT_EQ(through, 60);
}

TEST(SecondGate, AnUnevenPollNeverSkipsASecond)
{
    // The old timer ("a second since the last record") polled at 1.0-1.2 s and
    // lost a second every so often. Polling at 200 ms steps with jitter must not.
    SecondGate g;
    int through = 0;
    int64_t lastUtc = -1;
    for (uint32_t ms = 0; ms < 120000; ms += 200 + (ms % 3) * 10) {
        const int64_t utc = 7000 + ms / 1000;
        if (g.take(utc)) {
            ++through;
            if (lastUtc >= 0) {
                ASSERT_EQ(utc - lastUtc, 1) << "a second was skipped at " << ms << " ms";
            }
            lastUtc = utc;
        }
    }
    EXPECT_GE(through, 119);
}

TEST(SecondGate, AStallLetsOneThroughNotOnePerMissedSecond)
{
    SecondGate g;
    EXPECT_TRUE(g.take(100));
    EXPECT_TRUE(g.take(105));       // five seconds later
    EXPECT_FALSE(g.take(105));
    EXPECT_TRUE(g.take(106));
}

TEST(SecondGate, TheClockGoingBackwardsNeverRepeatsATime)
{
    SecondGate g;
    EXPECT_TRUE(g.take(100));
    EXPECT_FALSE(g.take(99));
    EXPECT_FALSE(g.take(100));
    EXPECT_TRUE(g.take(101));
}

TEST(SecondGate, ResetLetsTheNextOneThrough)
{
    SecondGate g;
    g.take(100);
    g.reset();
    EXPECT_TRUE(g.take(50));
}
