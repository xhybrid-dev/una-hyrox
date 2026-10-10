#include <gtest/gtest.h>

#include <cstring>

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
    EXPECT_STREQ(Text::reason(r), "Need 5 steady minutes");
    r.status = RunStatus::ProfileIncomplete;
    r.profileStatus = ProfileStatus::NeedsAge;
    EXPECT_STREQ(Text::reason(r), "Set your birth year");
    r.profileStatus = ProfileStatus::NeedsRestingHr;
    EXPECT_STREQ(Text::reason(r), "Set resting HR");
    r.profileStatus = ProfileStatus::ReserveTooSmall;
    EXPECT_STREQ(Text::reason(r), "Check HR settings");
}

TEST(Vo2Text, ReasonsFitOneLine)
{
    RunResult r;
    r.status = RunStatus::NotEnoughRunning;
    EXPECT_LE(std::strlen(Text::reason(r)), 21u);
    r.status = RunStatus::ProfileIncomplete;
    for (auto p : {ProfileStatus::NeedsAge, ProfileStatus::NeedsRestingHr, ProfileStatus::ReserveTooSmall}) {
        r.profileStatus = p;
        EXPECT_LE(std::strlen(Text::reason(r)), 21u);
    }
}
