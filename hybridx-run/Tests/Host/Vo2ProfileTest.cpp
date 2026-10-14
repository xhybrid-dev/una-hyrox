#include <gtest/gtest.h>

#include "Vo2Profile.hpp"

using namespace RunVo2;

namespace
{
// 2026-10-10 12:00:00 UTC.
constexpr std::time_t kNow = 1791633600;
}  // namespace

TEST(Vo2Profile, AgeCountsTheBirthMonth)
{
    EXPECT_EQ(ageAt(1986, 9, kNow), 40);    // September birthday has passed
    EXPECT_EQ(ageAt(1986, 10, kNow), 40);   // this month counts as passed
    EXPECT_EQ(ageAt(1986, 11, kNow), 39);   // not yet
    EXPECT_EQ(ageAt(1986, 0, kNow), 40);    // month unset: January
}

TEST(Vo2Profile, AgeUnknown)
{
    EXPECT_EQ(ageAt(0, 5, kNow), 0);
    EXPECT_EQ(ageAt(2030, 1, kNow), 0);
    EXPECT_EQ(ageAt(1990, 1, 0), 0);
}

TEST(Vo2Profile, TanakaRounds)
{
    EXPECT_EQ(tanakaMaxHr(40), 180);   // 208 - 28
    EXPECT_EQ(tanakaMaxHr(35), 184);   // 183.5 rounds up
    EXPECT_EQ(tanakaMaxHr(20), 194);
}

TEST(Vo2Profile, EnteredMaxWins)
{
    ProfileInput in;
    in.birthYear = 1986; in.birthMonth = 1;
    in.enteredMaxHr = 191; in.autoMaxHr = 200; in.watchRestHr = 50;
    const Profile p = resolveProfile(in, kNow);
    EXPECT_EQ(p.status, ProfileStatus::Ok);
    EXPECT_EQ(p.maxHr, 191);
    EXPECT_EQ(p.maxSource, MaxHrSource::Entered);
}

TEST(Vo2Profile, FormulaThenObservedWhenHigher)
{
    ProfileInput in;
    in.birthYear = 1986; in.birthMonth = 1; in.watchRestHr = 50;
    Profile p = resolveProfile(in, kNow);
    EXPECT_EQ(p.maxHr, 180);
    EXPECT_EQ(p.maxSource, MaxHrSource::Formula);

    in.autoMaxHr = 176;   // lower: the formula stands
    EXPECT_EQ(resolveProfile(in, kNow).maxHr, 180);

    in.autoMaxHr = 187;
    p = resolveProfile(in, kNow);
    EXPECT_EQ(p.maxHr, 187);
    EXPECT_EQ(p.maxSource, MaxHrSource::Observed);
}

TEST(Vo2Profile, NeedsAgeOrMax)
{
    ProfileInput in;
    in.watchRestHr = 50; in.autoMaxHr = 190;   // auto max alone is not enough
    EXPECT_EQ(resolveProfile(in, kNow).status, ProfileStatus::NeedsAge);

    in.enteredMaxHr = 185;   // no age needed with an entered max
    EXPECT_EQ(resolveProfile(in, kNow).status, ProfileStatus::Ok);
}

TEST(Vo2Profile, RestingHrSources)
{
    ProfileInput in;
    in.enteredMaxHr = 185;
    EXPECT_EQ(resolveProfile(in, kNow).status, ProfileStatus::NeedsRestingHr);

    in.watchRestHr = 55;
    EXPECT_EQ(resolveProfile(in, kNow).restHr, 55);

    in.enteredRestHr = 48;   // entered beats the watch
    EXPECT_EQ(resolveProfile(in, kNow).restHr, 48);

    in.enteredRestHr = 20;   // implausible: back to the watch
    EXPECT_EQ(resolveProfile(in, kNow).restHr, 55);
}

TEST(Vo2Profile, ReserveTooSmall)
{
    ProfileInput in;
    in.enteredMaxHr = 120; in.enteredRestHr = 90;
    EXPECT_EQ(resolveProfile(in, kNow).status, ProfileStatus::ReserveTooSmall);
}

TEST(Vo2Profile, RestingHrSourceIsReported)
{
    ProfileInput in;
    in.enteredMaxHr = 185;
    in.watchRestHr  = 55;
    EXPECT_EQ(resolveProfile(in, kNow).restSource, RestHrSource::Watch);
    in.enteredRestHr = 48;
    EXPECT_EQ(resolveProfile(in, kNow).restSource, RestHrSource::Entered);
    in.enteredRestHr = 0;
    in.watchRestHr   = 0;
    EXPECT_EQ(resolveProfile(in, kNow).restSource, RestHrSource::None);
}

TEST(Vo2Profile, CivilFromUtc)
{
    Civil c = civilFromUtc(kNow);
    EXPECT_EQ(c.year, 2026);
    EXPECT_EQ(c.month, 10);
    EXPECT_EQ(c.day, 10);
    EXPECT_EQ(c.hour, 12);
    c = civilFromUtc(0);
    EXPECT_EQ(c.year, 1970);
    EXPECT_EQ(c.month, 1);
    EXPECT_EQ(c.day, 1);
    c = civilFromUtc(1709164800);   // a leap day
    EXPECT_EQ(c.month, 2);
    EXPECT_EQ(c.day, 29);
    c = civilFromUtc(951782399);    // 2000-02-28T23:59:59Z
    EXPECT_EQ(c.day, 28);
    EXPECT_EQ(c.hour, 23);
    EXPECT_EQ(c.minute, 59);
    EXPECT_EQ(c.second, 59);
}
