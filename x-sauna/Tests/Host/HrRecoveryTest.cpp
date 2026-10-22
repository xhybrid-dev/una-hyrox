#include <gtest/gtest.h>

#include "HrRecovery.hpp"

using Sauna::Recovery;

TEST(HrRecovery, StartsNotBegun)
{
    Recovery r;
    EXPECT_EQ(r.state, Recovery::State::None);
}

TEST(HrRecovery, NoHeartRateAtTheEndIsNoResult)
{
    Recovery r;
    r.begin(1000, 0);
    EXPECT_EQ(r.state, Recovery::State::NoResult);
}

TEST(HrRecovery, ResolvesFromTheFirstSampleAtAMinute)
{
    Recovery r;
    r.begin(10000, 150);
    r.feed(40000, 140);                    // too early
    EXPECT_EQ(r.state, Recovery::State::Pending);
    r.feed(69999, 120);                    // 1 ms short
    EXPECT_EQ(r.state, Recovery::State::Pending);
    r.feed(70000, 122);
    EXPECT_EQ(r.state, Recovery::State::Done);
    EXPECT_EQ(r.drop, 28);
    r.feed(71000, 90);                     // later samples change nothing
    EXPECT_EQ(r.drop, 28);
}

TEST(HrRecovery, HeartRateThatRoseIsNegative)
{
    Recovery r;
    r.begin(0, 100);
    r.feed(60000, 112);
    EXPECT_EQ(r.drop, -12);
    EXPECT_EQ(r.state, Recovery::State::Done);
}

TEST(HrRecovery, NoHeartRateInTheWindowIsNoResult)
{
    Recovery r;
    r.begin(0, 150);
    r.feed(60000, 0);                      // a sample without a heart rate: keep waiting
    EXPECT_EQ(r.state, Recovery::State::Pending);
    r.feed(65000, 0);                      // the last moment of the window
    EXPECT_EQ(r.state, Recovery::State::Pending);
    r.feed(65001, 118);                    // too late to count
    EXPECT_EQ(r.state, Recovery::State::NoResult);
}

TEST(HrRecovery, WorksAcrossTheClockWrap)
{
    const uint32_t end = 0xFFFFFFFFu - 20000u;
    Recovery r;
    r.begin(end, 140);
    r.feed(end + 59000u, 100);             // wraps to a small number
    EXPECT_EQ(r.state, Recovery::State::Pending);
    r.feed(end + 61000u, 105);
    EXPECT_EQ(r.state, Recovery::State::Done);
    EXPECT_EQ(r.drop, 35);
}

TEST(HrRecovery, GiveUpOnlyAffectsAPendingOne)
{
    Recovery pending;
    pending.begin(0, 140);
    pending.giveUp();
    EXPECT_EQ(pending.state, Recovery::State::NoResult);

    Recovery done;
    done.begin(0, 140);
    done.feed(60000, 100);
    done.giveUp();
    EXPECT_EQ(done.state, Recovery::State::Done);

    Recovery none;
    none.giveUp();
    EXPECT_EQ(none.state, Recovery::State::None);
}

TEST(HrRecovery, DropIsClampedToTheInt16Range)
{
    Recovery r;
    r.begin(0, 65535);
    r.feed(60000, 1);
    EXPECT_EQ(r.drop, INT16_MAX);
}
