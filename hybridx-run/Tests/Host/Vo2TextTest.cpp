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
    r.status = RunStatus::TooEasy;
    EXPECT_STREQ(Text::reason(r), "Too easy to estimate");
    r.status = RunStatus::HrAboveMax;
    EXPECT_STREQ(Text::reason(r), "HR above your max");
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
    r.status = RunStatus::TooEasy;
    EXPECT_LE(std::strlen(Text::reason(r)), 21u);
    r.status = RunStatus::HrAboveMax;
    EXPECT_LE(std::strlen(Text::reason(r)), 21u);
    r.status = RunStatus::ProfileIncomplete;
    for (auto p : {ProfileStatus::NeedsAge, ProfileStatus::NeedsRestingHr, ProfileStatus::ReserveTooSmall}) {
        r.profileStatus = p;
        EXPECT_LE(std::strlen(Text::reason(r)), 21u);
    }
}

TEST(Vo2Text, Dates)
{
    char buf[12];
    EXPECT_STREQ(Text::shortDate(12, 10, buf, sizeof(buf)), "12 Oct");
    EXPECT_STREQ(Text::shortDate(1, 1, buf, sizeof(buf)), "1 Jan");
    EXPECT_STREQ(Text::shortDate(0, 1, buf, sizeof(buf)), "");
    EXPECT_STREQ(Text::shortDate(5, 13, buf, sizeof(buf)), "");
}

TEST(Vo2Text, Trend)
{
    char buf[32];
    EXPECT_STREQ(Text::trend(531, 523, buf, sizeof(buf)), "+0.8 since last run");
    EXPECT_STREQ(Text::trend(511, 523, buf, sizeof(buf)), "-1.2 since last run");
    EXPECT_STREQ(Text::trend(523, 523, buf, sizeof(buf)), "Same as last run");
    EXPECT_STREQ(Text::trend(523, 0, buf, sizeof(buf)), "");
    EXPECT_STREQ(Text::trend(0, 523, buf, sizeof(buf)), "");
    // Fits the overview's line (Regular 16, about 22 characters).
    EXPECT_LE(std::strlen(Text::trend(950, 150, buf, sizeof(buf))), 22u);
}

TEST(Vo2Text, Sources)
{
    EXPECT_STREQ(Text::maxSourceLabel(MaxHrSource::Formula), "age");
    EXPECT_STREQ(Text::maxSourceLabel(MaxHrSource::Observed), "runs");
    EXPECT_STREQ(Text::maxSourceLabel(MaxHrSource::Entered), "entered");
    EXPECT_STREQ(Text::maxSourceLabel(MaxHrSource::None), "");
    EXPECT_STREQ(Text::restSourceLabel(RestHrSource::Watch), "watch");
    EXPECT_STREQ(Text::restSourceLabel(RestHrSource::Entered), "entered");
    EXPECT_STREQ(Text::restSourceLabel(RestHrSource::None), "");
    EXPECT_STREQ(Text::profileReason(ProfileStatus::Ok), "");
    EXPECT_STREQ(Text::profileReason(ProfileStatus::NeedsAge), "Set your birth year");
}
