#include <gtest/gtest.h>

#include <cstring>

#include "Vo2Run.hpp"
#include "Vo2Window.hpp"
#include "Vo2WindowFile.hpp"

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

/// A window past the warm-up that passes every test, with the fields to bend.
WindowRecord goodWindow(uint32_t activeSec = 600)
{
    WindowRecord w;
    w.activeSec    = activeSec;
    w.goodSec      = 60;
    w.speedMMinX10 = 2000;   // 200 m/min
    w.spreadX10    = 20;
    w.gradeX10     = 0;
    w.hrX10        = 1600;
    w.hrMin        = 158;
    w.hrMax        = 162;
    return w;
}

}  // namespace

// ---- the builder ----------------------------------------------------------

TEST(Vo2Window, ABuilderMakesOneRecordPerMinute)
{
    WindowBuilder b;
    WindowRecord w;
    for (int i = 0; i < 59; ++i) {
        b.addSecond(steady());
        EXPECT_FALSE(b.popWindow(w));
    }
    b.addSecond(steady());
    ASSERT_TRUE(b.popWindow(w));
    EXPECT_FALSE(b.popWindow(w));   // once only
    EXPECT_EQ(w.index, 1);
    EXPECT_EQ(w.activeSec, 60u);
    EXPECT_EQ(w.goodSec, 60);
    EXPECT_EQ(w.speedMMinX10, 2000);
    EXPECT_EQ(w.hrX10, 1600);
    EXPECT_EQ(w.hrMin, 160);
    EXPECT_EQ(w.hrMax, 160);
    EXPECT_EQ(w.steepSec, 0);
    EXPECT_EQ(w.spreadX10, 0);
}

TEST(Vo2Window, ASecondOutsideTheGradeLimitsIsCountedNotJudged)
{
    WindowBuilder b;
    WindowRecord w;
    for (int i = 0; i < 60; ++i) {
        // One spike of +15% in an otherwise level minute.
        b.addSecond(steady(200.0f / 60.0f, 160, i == 30 ? 15.0f : 0.0f));
    }
    ASSERT_TRUE(b.popWindow(w));
    EXPECT_EQ(w.steepSec, 1);
    EXPECT_EQ(w.goodSec, 60);   // the second still counts as a good second
}

TEST(Vo2Window, AnInvalidGradeIsNotSteep)
{
    WindowBuilder b;
    WindowRecord w;
    Sample s = steady(200.0f / 60.0f, 160, 40.0f);
    s.gradeValid = false;
    for (int i = 0; i < 60; ++i) {
        b.addSecond(s);
    }
    ASSERT_TRUE(b.popWindow(w));
    EXPECT_EQ(w.steepSec, 0);
    EXPECT_EQ(w.gradeX10, 0);
}

TEST(Vo2Window, APauseBreaksTheWindowAndSustainedMaxNeedsAHold)
{
    WindowBuilder b;
    WindowRecord w;
    for (int i = 0; i < 40; ++i) {
        b.addSecond(steady());
    }
    Sample paused = steady();
    paused.active = false;
    b.addSecond(paused);
    for (int i = 0; i < 59; ++i) {
        b.addSecond(steady());
    }
    EXPECT_FALSE(b.popWindow(w));   // 59 seconds since the pause
    b.addSecond(steady());
    EXPECT_TRUE(b.popWindow(w));

    WindowBuilder m;
    for (int i = 0; i < 400; ++i) {
        m.addSecond(steady(200.0f / 60.0f, i == 350 ? 200 : 150));   // a one-second spike
    }
    EXPECT_EQ(m.sustainedMaxHr(), 150);
}

// ---- average grade-adjusted speed -------------------------------------------

TEST(Vo2Window, LevelRunningHasNoAdjustment)
{
    WindowBuilder b;
    for (int i = 0; i < 100; ++i) {
        b.addSecond(steady(3.0f));
    }
    EXPECT_NEAR(b.avgGradeAdjustedSpeedMs(), 3.0f, 1e-4f);
    EXPECT_EQ(b.movingSec(), 100u);
}

TEST(Vo2Window, UphillIsAdjustedByACSMsOwnGain)
{
    // 4% grade: VO2 = 3.5 + 0.2 v (1 + 4.5 x 0.04) = 3.5 + 0.2 v x 1.18.
    WindowBuilder b;
    for (int i = 0; i < 100; ++i) {
        b.addSecond(steady(3.0f, 160, 4.0f));
    }
    EXPECT_NEAR(b.avgGradeAdjustedSpeedMs(), 3.0f * 1.18f, 1e-3f);
}

TEST(Vo2Window, DownhillIsNotAdjustedAndASpikeIsClamped)
{
    WindowBuilder down;
    for (int i = 0; i < 50; ++i) {
        down.addSecond(steady(3.0f, 160, -6.0f));
    }
    EXPECT_NEAR(down.avgGradeAdjustedSpeedMs(), 3.0f, 1e-4f);

    WindowBuilder spike;
    spike.addSecond(steady(3.0f, 160, 80.0f));   // a sensor spike: counted as 20%
    EXPECT_NEAR(spike.avgGradeAdjustedSpeedMs(), 3.0f * (1.0f + 4.5f * 0.20f), 1e-3f);
}

TEST(Vo2Window, TheAverageIsOverMovingSecondsWithAValidSpeedOnly)
{
    WindowBuilder b;
    for (int i = 0; i < 10; ++i) {
        b.addSecond(steady(3.0f));
    }
    Sample standing = steady(0.2f);              // below the moving threshold
    Sample noSpeed  = steady(5.0f);
    noSpeed.speedValid = false;
    Sample dr = steady(5.0f);
    dr.deadReckoning = true;
    Sample paused = steady(5.0f);
    paused.active = false;
    for (const Sample& s : {standing, noSpeed, dr, paused}) {
        b.addSecond(s);
    }
    EXPECT_NEAR(b.avgGradeAdjustedSpeedMs(), 3.0f, 1e-4f);
    EXPECT_EQ(b.movingSec(), 10u);

    WindowBuilder none;
    EXPECT_EQ(none.avgGradeAdjustedSpeedMs(), 0.0f);
    b.reset();
    EXPECT_EQ(b.avgGradeAdjustedSpeedMs(), 0.0f);
    EXPECT_EQ(b.movingSec(), 0u);
}

TEST(Vo2Window, NoHeartRateIsNeededForTheAverage)
{
    Vo2Run run;
    Sample s = steady(3.0f);
    s.hrBpm   = 0;
    s.hrTrust = 0;
    for (int i = 0; i < 70; ++i) {
        run.addSecond(s);
    }
    EXPECT_NEAR(run.avgGradeAdjustedSpeedMs(), 3.0f, 1e-4f);
    EXPECT_EQ(run.movingSec(), 70u);
}

// ---- Run's judge on records ----------------------------------------------

TEST(Vo2Link, AWindowFromElsewhereIsJudgedLikeOneOfOurOwn)
{
    Vo2Run run;
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(run.addWindow(goodWindow(600 + 60 * i)), WindowOutcome::Kept);
    }
    const RunResult r = run.estimate(profile());
    ASSERT_EQ(r.status, RunStatus::Ok);
    EXPECT_EQ(r.vo2x10, 544);   // the same arithmetic as SteadyRunMatchesTheEquations
    EXPECT_EQ(run.counts().accepted, 5);
}

TEST(Vo2Link, TheWarmUpAndGapsAreStillDropped)
{
    Vo2Run run;
    WindowRecord w = goodWindow(300);
    EXPECT_EQ(run.addWindow(w), WindowOutcome::WarmUp);
    w = goodWindow();
    w.goodSec = 54;
    EXPECT_EQ(run.addWindow(w), WindowOutcome::Gaps);
}

TEST(Vo2Link, OneStraySecondDoesNotCostALevelMinute)
{
    Vo2Run run;
    WindowRecord w = goodWindow();
    w.steepSec = 1;
    EXPECT_EQ(run.addWindow(w), WindowOutcome::Kept);
    w.steepSec = Config::kMaxSteepSecPerWindow;
    EXPECT_EQ(run.addWindow(w), WindowOutcome::Kept);
    w.steepSec = Config::kMaxSteepSecPerWindow + 1;
    EXPECT_EQ(run.addWindow(w), WindowOutcome::Grade);
}

TEST(Vo2Link, AMeanOutsideTheLimitsIsSteepWhateverTheSeconds)
{
    Vo2Run run;
    WindowRecord w = goodWindow();
    w.gradeX10 = 101;    // 10.1%
    EXPECT_EQ(run.addWindow(w), WindowOutcome::Grade);
    w.gradeX10 = -31;    // -3.1%
    EXPECT_EQ(run.addWindow(w), WindowOutcome::Grade);
    w.gradeX10 = 100;    // the limits themselves are fine
    EXPECT_EQ(run.addWindow(w), WindowOutcome::Kept);
    w.gradeX10 = -30;
    EXPECT_EQ(run.addWindow(w), WindowOutcome::Kept);
}

TEST(Vo2Link, TheHeartRateRangeLimitIsTwelve)
{
    Vo2Run run;
    WindowRecord w = goodWindow();
    w.hrMin = 156;
    w.hrMax = 168;   // 12
    EXPECT_EQ(run.addWindow(w), WindowOutcome::Kept);
    w.hrMax = 169;   // 13
    EXPECT_EQ(run.addWindow(w), WindowOutcome::Unsteady);
}

TEST(Vo2Link, ASpikeInEveryMinuteNoLongerCostsTheWindows)
{
    // The real service path: a level run with one +15% second in each minute.
    // Before L1 every one of these minutes was rejected as steep.
    Vo2Run run;
    for (int i = 0; i < 30 * 60; ++i) {
        run.addSecond(steady(200.0f / 60.0f, 160, (i % 60) == 7 ? 15.0f : 0.0f));
    }
    const RunResult r = run.estimate(profile());
    ASSERT_EQ(r.status, RunStatus::Ok);
    // Each minute's mean grade carries the spike (+15% for one second = 0.25%),
    // which adds a little ACSM cost: 55.0, not 54.4. No window is rejected.
    EXPECT_EQ(r.vo2x10, 550);
    EXPECT_EQ(run.counts().grade, 0);
}

TEST(Vo2Link, SustainedMaxIsTheHigherOfOursAndTheOneNoted)
{
    Vo2Run run;
    run.noteSustainedMax(171);
    EXPECT_EQ(run.sustainedMaxHr(), 171);
    for (int i = 0; i < 400; ++i) {
        run.addSecond(steady(200.0f / 60.0f, 150));
    }
    EXPECT_EQ(run.sustainedMaxHr(), 171);
    run.reset();
    EXPECT_EQ(run.sustainedMaxHr(), 0);
}

// ---- Jon's run of 14 October 2026 (vo2_20261014T131357.csv) ----------------

namespace
{

struct Row { uint32_t act; uint16_t good, speed, spread; int16_t grade; uint16_t hr; uint8_t hmin, hmax; };

// activeSec, goodSec, speed m/min x10, spread % x10, grade % x10, HR x10, min, max.
// steepSec was not logged: taken as 0.
const Row kJonsRun[] = {
    {60, 48, 1707, 78, -3, 948, 73, 118},
    {120, 51, 1878, 26, 1, 1347, 120, 143},
    {180, 51, 1801, 40, 47, 1432, 140, 148},
    {240, 51, 1776, 49, 34, 1467, 142, 150},
    {300, 54, 1809, 31, -40, 1493, 149, 150},
    {360, 56, 1909, 20, -5, 1536, 149, 161},
    {420, 59, 1870, 18, 3, 1623, 160, 165},
    {480, 59, 1850, 14, 9, 1642, 163, 166},
    {540, 60, 1884, 40, -7, 1649, 163, 168},
    {600, 60, 1866, 26, -1, 1650, 162, 168},
    {660, 60, 1810, 21, -12, 1660, 162, 168},
    {720, 60, 1842, 19, -5, 1645, 156, 169},
    {780, 60, 1846, 21, -2, 1690, 165, 172},
    {840, 60, 1686, 78, 2, 1676, 163, 174},
    {900, 60, 1815, 13, 1, 1655, 162, 169},
    {960, 60, 1746, 25, 11, 1639, 162, 166},
    {1020, 60, 1651, 52, 19, 1622, 160, 164},
    {1080, 56, 1609, 46, 35, 1599, 159, 161},
    {1140, 59, 1729, 28, 8, 1597, 158, 161},
    {1200, 60, 1641, 40, 28, 1663, 161, 180},
    {1260, 60, 1649, 30, 23, 1613, 155, 167},
    {1320, 60, 1812, 40, -55, 1632, 160, 166},
    {1380, 60, 1901, 43, -33, 1617, 159, 163},
    {1440, 58, 1872, 26, 3, 1617, 160, 164},
    {1500, 60, 1805, 13, -3, 1627, 159, 167},
    {1560, 60, 1787, 16, 7, 1662, 162, 174},
    {1620, 59, 1775, 21, 5, 1732, 166, 177},
    {1680, 60, 1655, 45, 14, 1651, 160, 171},
    {1740, 60, 1715, 28, -2, 1632, 158, 169},
    {1800, 59, 1702, 44, 0, 1609, 159, 163},
    {1860, 58, 1815, 37, 1, 1618, 159, 168},
    {1920, 60, 1719, 35, 2, 1618, 159, 164},
    {1980, 60, 1805, 16, 1, 1641, 162, 167},
    {2040, 60, 1755, 37, 2, 1638, 160, 166},
    {2100, 60, 1707, 36, 9, 1647, 162, 168},
    {2160, 60, 1690, 54, 0, 1643, 162, 168},
    {2220, 59, 1718, 35, 4, 1588, 152, 163},
    {2280, 56, 1658, 24, 13, 1513, 150, 154},
    {2340, 55, 1476, 27, 45, 1509, 149, 152},
    {2400, 42, 1598, 72, 42, 1511, 149, 155},
    {2460, 55, 1735, 19, -11, 1545, 150, 159},
    {2520, 60, 1607, 34, 21, 1614, 157, 164},
    {2580, 35, 1499, 55, 33, 1622, 157, 165},
    {2640, 30, 1455, 50, 37, 1660, 164, 171},
    {2700, 60, 1654, 40, -22, 1630, 159, 167},
    {2760, 60, 1770, 7, -28, 1628, 160, 166},
    {2820, 60, 1812, 10, -31, 1626, 159, 166},
};

}  // namespace

// The log's own result was 45.2 from 22 windows under the old rules (HR range
// 10, any steep second rejects the window). The same windows under the L1
// rules: 34 windows, 45.1 (docs/experiments/vo2_log_whatif.py says the same).
TEST(Vo2Link, JonsRunUnderTheNewRules)
{
    Vo2Run run;
    for (const Row& r : kJonsRun) {
        WindowRecord w;
        w.activeSec    = r.act;
        w.goodSec      = r.good;
        w.speedMMinX10 = static_cast<uint16_t>(r.speed);
        w.spreadX10    = r.spread;
        w.gradeX10     = r.grade;
        w.hrX10        = r.hr;
        w.hrMin        = r.hmin;
        w.hrMax        = r.hmax;
        run.addWindow(w);
    }
    EXPECT_EQ(run.counts().accepted, 34);
    const RunResult r = run.estimate(profile(182, 41));
    ASSERT_EQ(r.status, RunStatus::Ok);
    EXPECT_EQ(r.windowsUsed, 34);
    EXPECT_NEAR(r.vo2x10, 451, 2);

    // P3, measured first (the figures from an independent Python reading of the same rules):
    // two of the 34 minutes followed a change of pace, and without them the estimate is the same;
    // the first half of the minutes reads 45.9 and the second 44.1 (cardiac drift); a typical minute
    // sits 1.2 from the middle.
    EXPECT_EQ(run.counts().afterChange, 2);
    EXPECT_EQ(run.counts().lagged, 0);   // the guard is off: they were used
    EXPECT_EQ(r.afterChange, 2);
    EXPECT_NEAR(r.withoutLagX10, 451, 1);
    EXPECT_NEAR(r.firstHalfX10, 459, 1);
    EXPECT_NEAR(r.secondHalfX10, 441, 1);
    EXPECT_NEAR(r.spreadX10, 12, 1);
}

// ---- the shared file --------------------------------------------------------

TEST(Vo2WindowFile, RoundTrip)
{
    WindowRecord in[3] = {goodWindow(360), goodWindow(420), goodWindow(480)};
    in[1].gradeX10 = -27;
    in[1].steepSec = 4;
    in[2].hrX10    = 1655;
    char buf[kWindowFileBytes];
    const size_t n = writeWindowFile(buf, sizeof(buf), 1791980037u, 179, in, 3);
    ASSERT_GT(n, 0u);

    WindowFileReader rd(buf, n);
    WindowFileInfo info;
    ASSERT_TRUE(rd.open(info));
    EXPECT_EQ(info.runId, 1791980037u);
    EXPECT_EQ(info.maxHr, 179);
    EXPECT_EQ(info.count, 3);
    for (uint16_t i = 0; i < 3; ++i) {
        WindowRecord out;
        ASSERT_TRUE(rd.window(i, out)) << i;
        EXPECT_EQ(out.index, i + 1);
        EXPECT_EQ(out.activeSec, in[i].activeSec);
        EXPECT_EQ(out.goodSec, in[i].goodSec);
        EXPECT_EQ(out.speedMMinX10, in[i].speedMMinX10);
        EXPECT_EQ(out.spreadX10, in[i].spreadX10);
        EXPECT_EQ(out.gradeX10, in[i].gradeX10);
        EXPECT_EQ(out.steepSec, in[i].steepSec);
        EXPECT_EQ(out.hrX10, in[i].hrX10);
        EXPECT_EQ(out.hrMin, in[i].hrMin);
        EXPECT_EQ(out.hrMax, in[i].hrMax);
    }
    WindowRecord none;
    EXPECT_FALSE(rd.window(3, none));
}

TEST(Vo2WindowFile, TheWorstCaseFits)
{
    static WindowRecord many[kWindowFileMaxWindows + 10];
    for (auto& w : many) {
        w.activeSec    = 999999;
        w.goodSec      = 60;
        w.speedMMinX10 = 6000;
        w.spreadX10    = 6000;
        w.gradeX10     = -3000;
        w.steepSec     = 60;
        w.hrX10        = 2300;
        w.hrMin        = 255;
        w.hrMax        = 255;
    }
    char buf[kWindowFileBytes];
    const size_t n = writeWindowFile(buf, sizeof(buf), 0xFFFFFFFFu, 230, many, kWindowFileMaxWindows + 10);
    ASSERT_GT(n, 0u);
    EXPECT_LT(n, kWindowFileBytes);
    WindowFileReader rd(buf, n);
    WindowFileInfo info;
    ASSERT_TRUE(rd.open(info));
    EXPECT_EQ(info.count, kWindowFileMaxWindows);   // capped, the first ones kept
}

TEST(Vo2WindowFile, BadArgumentsAndSmallBuffersWriteNothing)
{
    WindowRecord w = goodWindow();
    char buf[kWindowFileBytes];
    EXPECT_EQ(writeWindowFile(nullptr, 100, 1, 0, &w, 1), 0u);
    EXPECT_EQ(writeWindowFile(buf, 0, 1, 0, &w, 1), 0u);
    EXPECT_EQ(writeWindowFile(buf, sizeof(buf), 0, 0, &w, 1), 0u);        // no run id
    EXPECT_EQ(writeWindowFile(buf, sizeof(buf), 1, 0, nullptr, 1), 0u);
    char tiny[40];
    EXPECT_EQ(writeWindowFile(tiny, sizeof(tiny), 1, 0, &w, 1), 0u);      // does not fit
    EXPECT_EQ(tiny[0], '\0');
}

TEST(Vo2WindowFile, MalformedAndNewerFilesReadAsNothing)
{
    const char* bad[] = {
        "",
        "not json",
        "{}",
        "{\"v\":2,\"id\":1,\"max\":0,\"w\":[]}",            // a newer version
        "{\"v\":1,\"id\":0,\"max\":0,\"w\":[]}",            // no run id
        "{\"v\":1,\"id\":1,\"max\":231,\"w\":[]}",          // max HR out of range
        "{\"v\":1,\"id\":1,\"max\":0}",                       // no windows array
        "{\"v\":1,\"id\":1,\"max\":0,\"w\":[[1,2,3]]",      // truncated
    };
    for (const char* t : bad) {
        WindowFileReader rd(t, std::strlen(t));
        WindowFileInfo info;
        EXPECT_FALSE(rd.open(info)) << t;
    }
}

TEST(Vo2WindowFile, OutOfRangeWindowsAreRefused)
{
    // [activeSec, good, speed, spread, grade, steep, hr, hrMin, hrMax]
    const char* bad[] = {
        "{\"v\":1,\"id\":1,\"max\":0,\"w\":[[600,61,2000,20,0,0,1600,158,162]]}",    // 61 good seconds
        "{\"v\":1,\"id\":1,\"max\":0,\"w\":[[600,60,99999,20,0,0,1600,158,162]]}",   // speed
        "{\"v\":1,\"id\":1,\"max\":0,\"w\":[[600,60,2000,20,6001,0,1600,158,162]]}", // grade
        "{\"v\":1,\"id\":1,\"max\":0,\"w\":[[600,60,2000,20,0,0,1600,170,162]]}",    // min > max
        "{\"v\":1,\"id\":1,\"max\":0,\"w\":[[600,60,2000,20,0,0,1600,158]]}",        // short
        "{\"v\":1,\"id\":1,\"max\":0,\"w\":[[-5,60,2000,20,0,0,1600,158,162]]}",     // negative
    };
    for (const char* t : bad) {
        WindowFileReader rd(t, std::strlen(t));
        WindowFileInfo info;
        ASSERT_TRUE(rd.open(info)) << t;
        WindowRecord w;
        EXPECT_FALSE(rd.window(0, w)) << t;
    }
}

TEST(Vo2WindowFile, TrailOverwritesAnEmptySlotThenTheOldestRun)
{
    const uint32_t empty[3]   = {0, 0, 0};
    EXPECT_EQ(windowFileSlotToWrite(empty), 0);
    const uint32_t one[3]     = {500, 0, 0};
    EXPECT_EQ(windowFileSlotToWrite(one), 1);
    const uint32_t gap[3]     = {500, 600, 0};
    EXPECT_EQ(windowFileSlotToWrite(gap), 2);
    const uint32_t full[3]    = {500, 300, 400};
    EXPECT_EQ(windowFileSlotToWrite(full), 1);
    const uint32_t full2[3]   = {200, 300, 100};
    EXPECT_EQ(windowFileSlotToWrite(full2), 2);
    const uint32_t middle[3]  = {900, 0, 100};   // an unreadable slot is reused first
    EXPECT_EQ(windowFileSlotToWrite(middle), 1);
}

TEST(Vo2WindowFile, AFileFlowsThroughTheJudgeToTheSameFigure)
{
    // Trail side: build windows from seconds, write them. Run side: read, judge.
    WindowBuilder b;
    static WindowRecord recs[40];
    uint16_t n = 0;
    for (int i = 0; i < 30 * 60; ++i) {
        b.addSecond(steady());
        WindowRecord w;
        if (b.popWindow(w) && w.activeSec > Config::kWarmUpSec && n < 40) {
            recs[n++] = w;
        }
    }
    char buf[kWindowFileBytes];
    const size_t len = writeWindowFile(buf, sizeof(buf), 1791980037u, b.sustainedMaxHr(), recs, n);
    ASSERT_GT(len, 0u);

    WindowFileReader rd(buf, len);
    WindowFileInfo info;
    ASSERT_TRUE(rd.open(info));
    Vo2Run run;
    run.noteSustainedMax(info.maxHr);
    for (uint16_t i = 0; i < info.count; ++i) {
        WindowRecord w;
        ASSERT_TRUE(rd.window(i, w));
        run.addWindow(w);
    }
    const RunResult r = run.estimate(profile());
    ASSERT_EQ(r.status, RunStatus::Ok);
    EXPECT_EQ(r.vo2x10, 544);
    EXPECT_EQ(run.sustainedMaxHr(), 160);
}
