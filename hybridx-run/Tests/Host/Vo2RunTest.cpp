#include <gtest/gtest.h>

#include "Vo2Run.hpp"

using namespace RunVo2;

namespace
{

Profile profile(uint8_t maxHr = 190, uint8_t restHr = 50)
{
    Profile p;
    p.status = ProfileStatus::Ok;
    p.maxHr  = maxHr;
    p.restHr = restHr;
    return p;
}

Sample steady(float speedMs = 200.0f / 60.0f, uint8_t hr = 160, float grade = 0.0f)
{
    Sample s;
    s.active     = true;
    s.speedMs    = speedMs;
    s.speedValid = true;
    s.gradePct   = grade;
    s.gradeValid = true;
    s.hrBpm      = hr;
    s.hrTrust    = 2;
    return s;
}

void feed(Vo2Run& run, const Sample& s, int seconds)
{
    for (int i = 0; i < seconds; ++i) {
        run.addSecond(s);
    }
}

}  // namespace

// 200 m/min level: ACSM cost 3.5 + 40 = 43.5. HR 160 with rest 50, max 190:
// %HRR 110/140. VO2max = 3.5 + 40 x 140/110 = 54.41.
TEST(Vo2Run, SteadyRunMatchesTheEquations)
{
    Vo2Run run;
    feed(run, steady(), 30 * 60);
    const RunResult r = run.estimate(profile());
    ASSERT_EQ(r.status, RunStatus::Ok);
    EXPECT_EQ(r.vo2x10, 544);
    EXPECT_EQ(r.windowsUsed, 25);
    EXPECT_EQ(run.counts().warmUp, 5);
    EXPECT_EQ(run.counts().accepted, 25);
}

TEST(Vo2Run, FasterAtTheSameHeartRateIsFitter)
{
    Vo2Run slow, fast;
    feed(slow, steady(3.0f), 20 * 60);
    feed(fast, steady(3.6f), 20 * 60);
    EXPECT_GT(fast.estimate(profile()).vo2x10, slow.estimate(profile()).vo2x10);
}

TEST(Vo2Run, UphillCostsMoreDownhillCountsAsLevel)
{
    Vo2Run level, up, down;
    feed(level, steady(3.0f, 160, 0.0f), 20 * 60);
    feed(up, steady(3.0f, 160, 4.0f), 20 * 60);
    feed(down, steady(3.0f, 160, -2.0f), 20 * 60);
    const uint16_t l = level.estimate(profile()).vo2x10;
    EXPECT_GT(up.estimate(profile()).vo2x10, l);
    EXPECT_EQ(down.estimate(profile()).vo2x10, l);
}

TEST(Vo2Run, SteepWindowsAreDropped)
{
    Vo2Run run;
    feed(run, steady(3.0f, 160, 12.0f), 20 * 60);
    EXPECT_EQ(run.estimate(profile()).status, RunStatus::NotEnoughRunning);
    EXPECT_EQ(run.counts().grade, 15);

    Vo2Run steepDown;
    feed(steepDown, steady(3.0f, 160, -5.0f), 20 * 60);
    EXPECT_EQ(steepDown.counts().grade, 15);
}

TEST(Vo2Run, ShortRunHasNoEstimate)
{
    Vo2Run run;
    feed(run, steady(), 9 * 60);   // 5 warm-up windows, 4 usable
    const RunResult r = run.estimate(profile());
    EXPECT_EQ(r.status, RunStatus::NotEnoughRunning);
    EXPECT_EQ(r.windowsUsed, 4);
}

TEST(Vo2Run, LowTrustOrDeadReckoningAreGaps)
{
    Sample bad = steady();
    bad.hrTrust = 0;
    Vo2Run a;
    feed(a, bad, 20 * 60);
    EXPECT_EQ(a.counts().gaps, 15);

    bad = steady();
    bad.deadReckoning = true;
    Vo2Run b;
    feed(b, bad, 20 * 60);
    EXPECT_EQ(b.counts().gaps, 15);

    bad = steady(2.0f);   // walking pace
    Vo2Run c;
    feed(c, bad, 20 * 60);
    EXPECT_EQ(c.counts().gaps, 15);
}

TEST(Vo2Run, AFewMissingSecondsAreTolerated)
{
    Vo2Run run;
    Sample miss = steady();
    miss.speedValid = false;
    for (int m = 0; m < 20; ++m) {
        feed(run, steady(), 56);
        feed(run, miss, 4);
    }
    EXPECT_EQ(run.counts().accepted, 15);
}

TEST(Vo2Run, UnsteadyWindowsAreDropped)
{
    Vo2Run speed;
    for (int i = 0; i < 20 * 60; ++i) {
        speed.addSecond(steady(i % 2 ? 2.6f : 3.6f));
    }
    EXPECT_EQ(speed.counts().unsteady, 15);

    Vo2Run hr;
    for (int i = 0; i < 20 * 60; ++i) {
        hr.addSecond(steady(3.3f, static_cast<uint8_t>(150 + (i % 60) / 4)));   // 150..164
    }
    EXPECT_EQ(hr.counts().unsteady, 15);
}

TEST(Vo2Run, PauseBreaksTheWindow)
{
    Vo2Run run;
    feed(run, steady(), 6 * 60);   // past the warm-up
    feed(run, steady(), 30);
    Sample paused = steady();
    paused.active = false;
    feed(run, paused, 120);
    feed(run, steady(), 60);
    // The 30 s before the pause are dropped, not merged with what follows.
    EXPECT_EQ(run.counts().accepted, 2);
    EXPECT_EQ(run.counts().gaps, 0);
}

TEST(Vo2Run, EasyEffortIsNotUsed)
{
    Vo2Run run;
    feed(run, steady(3.0f, 110), 20 * 60);   // %HRR 60/140 = 43%
    const RunResult r = run.estimate(profile());
    EXPECT_EQ(r.status, RunStatus::NotEnoughRunning);
    EXPECT_EQ(r.windowsUsed, 0);
    EXPECT_EQ(run.counts().accepted, 15);   // kept; the profile decides
}

TEST(Vo2Run, IncompleteProfile)
{
    Vo2Run run;
    feed(run, steady(), 20 * 60);
    Profile p;
    p.status = ProfileStatus::NeedsRestingHr;
    const RunResult r = run.estimate(p);
    EXPECT_EQ(r.status, RunStatus::ProfileIncomplete);
    EXPECT_EQ(r.profileStatus, ProfileStatus::NeedsRestingHr);
}

TEST(Vo2Run, MedianResistsOutliers)
{
    Vo2Run run;
    feed(run, steady(3.3f, 160), 15 * 60);   // 10 good windows
    feed(run, steady(3.3f, 120), 2 * 60);    // 2 low-HR windows: very high estimates
    const RunResult r = run.estimate(profile());
    Vo2Run clean;
    feed(clean, steady(3.3f, 160), 15 * 60);
    EXPECT_EQ(r.vo2x10, clean.estimate(profile()).vo2x10);
}

TEST(Vo2Run, SustainedMaxNeedsAHold)
{
    Vo2Run run;
    feed(run, steady(3.3f, 170), 60);
    feed(run, steady(3.3f, 199), 3);    // spike: too short
    feed(run, steady(3.3f, 170), 10);
    EXPECT_EQ(run.sustainedMaxHr(), 170);
    feed(run, steady(3.3f, 186), 5);
    EXPECT_EQ(run.sustainedMaxHr(), 186);

    Sample low = steady(3.3f, 195);
    low.hrTrust = 4;                    // untrusted: never counts
    feed(run, low, 30);
    EXPECT_EQ(run.sustainedMaxHr(), 186);
}

TEST(Vo2Run, ResetForgets)
{
    Vo2Run run;
    feed(run, steady(), 20 * 60);
    run.reset();
    EXPECT_EQ(run.counts().accepted, 0);
    EXPECT_EQ(run.sustainedMaxHr(), 0);
}
