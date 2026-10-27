/**
 * P3: the VO2max reliability guards, measured first. While their switches are off they must
 * change nothing in the estimate, and say what they would have changed.
 */

#include <gtest/gtest.h>

#include "Vo2History.hpp"
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

Sample steady(float speedMs, uint8_t hr)
{
    Sample s;
    s.active     = true;
    s.speedMs    = speedMs;
    s.speedValid = true;
    s.gradeValid = true;
    s.hrBpm      = hr;
    s.hrTrust    = 2;
    return s;
}

void feed(Vo2Run& run, const Sample& s, int seconds)
{
    for (int i = 0; i < seconds; ++i) {
        run.addSecond(s);
        WindowLog ignored;   // drained, so the next counting feed sees only its own windows
        run.popWindowLog(ignored);
    }
}

/// Feed and collect the windows' log entries.
int feedCountingFlags(Vo2Run& run, const Sample& s, int seconds)
{
    int flagged = 0;
    for (int i = 0; i < seconds; ++i) {
        run.addSecond(s);
        WindowLog w;
        if (run.popWindowLog(w) && w.afterChange) {
            ++flagged;
        }
    }
    return flagged;
}

}  // namespace

static_assert(!Config::kLagGuardEnabled, "P3: the lag guard stays off until Jon has compared with his Garmin");

TEST(Vo2Guard, ASteadyRunHasNoMinuteAfterAChange)
{
    // The first window after the warm-up follows the warm-up's last minute, at the same pace.
    Vo2Run run;
    feed(run, steady(3.3f, 160), 30 * 60);
    const RunResult r = run.estimate(profile());
    ASSERT_EQ(r.status, RunStatus::Ok);
    EXPECT_EQ(r.afterChange, 0);
    EXPECT_EQ(r.withoutLagX10, r.vo2x10);
    EXPECT_EQ(r.spreadX10, 0);   // every minute the same
}

TEST(Vo2Guard, TheMinuteAfterAPaceChangeIsFlaggedButStillUsed)
{
    // Ten minutes easy, then twenty a fifth faster. In the first faster minute the heart rate has
    // not caught up (it reads as the old rate), so that minute alone reads high.
    Vo2Run run;
    feed(run, steady(3.0f, 150), 10 * 60);
    int flagged = feedCountingFlags(run, steady(3.6f, 150), 60);   // HR lagging
    flagged += feedCountingFlags(run, steady(3.6f, 170), 19 * 60);
    EXPECT_EQ(flagged, 1);
    EXPECT_EQ(run.counts().afterChange, 1);
    EXPECT_EQ(run.counts().lagged, 0);

    const RunResult r = run.estimate(profile());
    ASSERT_EQ(r.status, RunStatus::Ok);
    EXPECT_EQ(r.afterChange, 1);
    // The estimate is the median over all 25 minutes, the lagging one included; without it, 24.
    EXPECT_EQ(r.windowsUsed, 25);
    EXPECT_GT(r.withoutLagX10, 0);
    EXPECT_LE(r.withoutLagX10, r.vo2x10);   // dropping a high minute can only lower the median
}

TEST(Vo2Guard, ASmallChangeOfPaceIsNotAChange)
{
    // 5% faster is inside the 10% tolerance.
    Vo2Run run;
    feed(run, steady(3.0f, 150), 10 * 60);
    EXPECT_EQ(feedCountingFlags(run, steady(3.15f, 152), 10 * 60), 0);
}

TEST(Vo2Guard, TheMinuteAfterAPauseIsFlagged)
{
    // Ten minutes, half a minute more, a pause, then on at the same pace: the part-built minute
    // is dropped, so the next window does not follow on from the last one.
    Vo2Run run;
    feed(run, steady(3.3f, 160), 10 * 60 + 30);
    Sample paused = steady(3.3f, 160);
    paused.active = false;
    feed(run, paused, 120);
    EXPECT_EQ(feedCountingFlags(run, steady(3.3f, 160), 10 * 60), 1);
}

TEST(Vo2Guard, AMinuteWithoutSpeedMakesTheNextOneAChange)
{
    Vo2Run run;
    feed(run, steady(3.3f, 160), 10 * 60);
    Sample noGps = steady(3.3f, 160);
    noGps.speedValid = false;
    feed(run, noGps, 60);   // one minute with no speed at all (itself flagged, and rejected for gaps)
    EXPECT_EQ(feedCountingFlags(run, steady(3.3f, 160), 5 * 60), 1);   // only the minute after it
}

TEST(Vo2Guard, ResetForgetsTheMinuteBefore)
{
    Vo2Run run;
    feed(run, steady(3.0f, 150), 10 * 60);
    run.reset();
    // A new run: its first window has no minute before (warm-up, so not counted anyway).
    feed(run, steady(3.6f, 170), 30 * 60);
    EXPECT_EQ(run.counts().afterChange, 0);
}

TEST(Vo2Guard, TheHalvesShowCardiacDrift)
{
    // The same pace throughout, the heart rate creeping up: the second half reads lower.
    Vo2Run run;
    feed(run, steady(3.3f, 155), 5 * 60);   // the warm-up
    for (int m = 0; m < 30; ++m) {
        feed(run, steady(3.3f, static_cast<uint8_t>(155 + m / 3)), 60);
    }
    const RunResult r = run.estimate(profile());
    ASSERT_EQ(r.status, RunStatus::Ok);
    EXPECT_GT(r.firstHalfX10, r.secondHalfX10);
    EXPECT_GE(r.firstHalfX10, r.vo2x10);
    EXPECT_LE(r.secondHalfX10, r.vo2x10);
    EXPECT_GT(r.spreadX10, 0);
}

TEST(Vo2Guard, TooFewMinutesForHalvesGiveNone)
{
    // Five windows: an estimate, but halves of two and three are too few for a median each.
    Vo2Run run;
    feed(run, steady(3.3f, 160), 10 * 60);
    const RunResult r = run.estimate(profile());
    ASSERT_EQ(r.status, RunStatus::Ok);
    EXPECT_EQ(r.windowsUsed, 5);
    EXPECT_EQ(r.firstHalfX10, 0);
    EXPECT_EQ(r.secondHalfX10, 0);
}

TEST(Vo2Guard, WithoutLagNeedsEnoughCleanMinutes)
{
    // Seven used minutes, three of them after changes of pace: four clean ones are too few.
    Vo2Run run;
    feed(run, steady(3.0f, 150), 6 * 60);   // warm-up + 1 used
    feed(run, steady(3.6f, 168), 60);       // change
    feed(run, steady(3.0f, 150), 60);       // change
    feed(run, steady(3.6f, 168), 60);       // change
    feed(run, steady(3.6f, 168), 3 * 60);   // three clean
    const RunResult r = run.estimate(profile());
    ASSERT_EQ(r.status, RunStatus::Ok);
    EXPECT_EQ(r.afterChange, 3);
    EXPECT_EQ(r.withoutLagX10, 0);
}

// ---- the shown value's age limit, measured only --------------------------------------------

TEST(Vo2Guard, TheAgeLimitedShownValueCountsOnlyRecentRuns)
{
    const uint32_t day = 86400u;
    const uint32_t now = 1793000000u;
    Vo2History h;
    h.add(RunRecord{now - 90 * day, 400, 30});   // three months old
    h.add(RunRecord{now - 60 * day, 420, 30});   // two months old
    h.add(RunRecord{now - 10 * day, 500, 30});
    h.add(RunRecord{now - 2 * day, 520, 30});
    EXPECT_EQ(h.rollingX10(), (400 + 420 + 500 + 520) / 4);   // all four: unchanged
    EXPECT_EQ(h.rollingX10Within(now), 510);                  // the last 42 days only
}

TEST(Vo2Guard, NoRecentRunMeansNoAgeLimitedValue)
{
    const uint32_t now = 1793000000u;
    Vo2History h;
    h.add(RunRecord{now - 100u * 86400u, 450, 20});
    EXPECT_EQ(h.rollingX10(), 450);
    EXPECT_EQ(h.rollingX10Within(now), 0);
}

TEST(Vo2Guard, AllRecentRunsGiveTheSameShownValue)
{
    const uint32_t now = 1793000000u;
    Vo2History h;
    for (int i = 0; i < 7; ++i) {
        h.add(RunRecord{now - static_cast<uint32_t>(7 - i) * 86400u, static_cast<uint16_t>(480 + i * 5),
                        static_cast<uint16_t>(10 + i)});
    }
    EXPECT_EQ(h.rollingX10Within(now), h.rollingX10());
}
