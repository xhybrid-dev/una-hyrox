#include <gtest/gtest.h>

#include "Vo2Text.hpp"

using namespace RunVo2;

TEST(Vo2Text, Format)
{
    char buf[8];
    EXPECT_STREQ(Text::formatX10(523, buf, sizeof(buf)), "52.3");
    EXPECT_STREQ(Text::formatX10(500, buf, sizeof(buf)), "50.0");
    EXPECT_STREQ(Text::formatX10(0, buf, sizeof(buf)), "---");
    EXPECT_STREQ(Text::formatX10(523, buf, 3), "52");   // truncated, terminated
}

TEST(Vo2Text, Reasons)
{
    RunResult r;
    r.status = RunStatus::Ok;
    EXPECT_STREQ(Text::reason(r), "");
    r.status = RunStatus::NotEnoughRunning;
    EXPECT_STREQ(Text::reason(r), "Not enough steady running");
    r.status = RunStatus::ProfileIncomplete;
    r.profileStatus = ProfileStatus::NeedsAge;
    EXPECT_STREQ(Text::reason(r), "Set birth year or max HR");
    r.profileStatus = ProfileStatus::NeedsRestingHr;
    EXPECT_STREQ(Text::reason(r), "Set resting HR");
    r.profileStatus = ProfileStatus::ReserveTooSmall;
    EXPECT_STREQ(Text::reason(r), "Check max and resting HR");
}
