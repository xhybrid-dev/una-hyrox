#include <gtest/gtest.h>

#include "LapLockout.hpp"

using Workout::kLapLockoutMs;
using Workout::LapLockout;

TEST(LapLockout, TheFirstPressCounts)
{
    LapLockout l;
    EXPECT_TRUE(l.allow(5000));
}

TEST(LapLockout, APressInsideTheLockoutIsIgnored)
{
    LapLockout l;
    EXPECT_TRUE(l.allow(1000));
    EXPECT_FALSE(l.allow(1000));
    EXPECT_FALSE(l.allow(1000 + kLapLockoutMs - 1));
}

TEST(LapLockout, APressAfterTheLockoutCounts)
{
    LapLockout l;
    EXPECT_TRUE(l.allow(1000));
    EXPECT_TRUE(l.allow(1000 + kLapLockoutMs));
}

TEST(LapLockout, AnIgnoredPressDoesNotExtendTheLockout)
{
    LapLockout l;
    EXPECT_TRUE(l.allow(0));
    EXPECT_FALSE(l.allow(2000));            // ignored: must not restart the 3 s
    EXPECT_TRUE(l.allow(kLapLockoutMs));
}

TEST(LapLockout, WorksAcrossTheClockWrap)
{
    LapLockout l;
    const uint32_t t = 0xFFFFFFFFu - 1000u;
    EXPECT_TRUE(l.allow(t));
    EXPECT_FALSE(l.allow(t + 2000u));       // wraps to a small number
    EXPECT_TRUE(l.allow(t + 3500u));
}

TEST(LapLockout, ResetLetsTheNextPressThrough)
{
    LapLockout l;
    l.allow(100);
    l.reset();
    EXPECT_TRUE(l.allow(101));
}
