// The Run link, from Trail's side: Trail turns a run's seconds into window
// records and writes them for HybridX Run. The builder and codec are copies of
// Run's (Software/Libs/Vo2Link, checked by CI); Run's own host tests cover the
// judging and the reading. This covers what Trail does with them.
#include <gtest/gtest.h>

#include <cstring>

#include "Vo2Window.hpp"
#include "Vo2WindowFile.hpp"

using namespace RunVo2;

namespace
{

Sample steady(float speedMs = 3.0f, uint8_t hr = 150, float grade = 0.0f)
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

}  // namespace

// What Service::feedVo2 / saveVo2Windows do, in miniature: keep the minutes past
// the warm-up with 40+ good seconds, write them, read them back.
TEST(TrailVo2Link, ARunBecomesAFileRunCanRead)
{
    WindowBuilder b;
    WindowRecord  kept[kWindowFileMaxWindows];
    uint16_t      n = 0;
    for (int i = 0; i < 12 * 60; ++i) {
        b.addSecond(steady());
        WindowRecord w;
        if (b.popWindow(w) && w.activeSec > Config::kWarmUpSec && w.goodSec >= 40 && n < kWindowFileMaxWindows) {
            kept[n++] = w;
        }
    }
    EXPECT_EQ(n, 7);   // minutes 6 to 12; the first five are the warm-up

    char buf[kWindowFileBytes];
    const size_t len = writeWindowFile(buf, sizeof(buf), 1792085390u, b.sustainedMaxHr(), kept, n);
    ASSERT_GT(len, 0u);

    WindowFileReader rd(buf, len);
    WindowFileInfo   info;
    ASSERT_TRUE(rd.open(info));
    EXPECT_EQ(info.runId, 1792085390u);
    EXPECT_EQ(info.maxHr, 150);
    EXPECT_EQ(info.count, 7);
    WindowRecord w;
    ASSERT_TRUE(rd.window(0, w));
    EXPECT_EQ(w.activeSec, 360u);
    EXPECT_EQ(w.speedMMinX10, 1800);   // 3 m/s
    EXPECT_EQ(w.hrX10, 1500);
}

TEST(TrailVo2Link, ALongRunKeepsTheFirstTwoHours)
{
    static WindowRecord many[kWindowFileMaxWindows + 30];
    for (auto& w : many) {
        w.activeSec = 400;
        w.goodSec   = 60;
    }
    char buf[kWindowFileBytes];
    const size_t len = writeWindowFile(buf, sizeof(buf), 5u, 0, many, kWindowFileMaxWindows + 30);
    ASSERT_GT(len, 0u);
    WindowFileReader rd(buf, len);
    WindowFileInfo   info;
    ASSERT_TRUE(rd.open(info));
    EXPECT_EQ(info.count, kWindowFileMaxWindows);
}

TEST(TrailVo2Link, TheOldestSlotIsOverwrittenFirst)
{
    // Three saved runs, then a fourth: the oldest goes, then the next oldest.
    uint32_t ids[kWindowFileSlots] = {0, 0, 0};
    const uint32_t runs[] = {1000, 2000, 3000, 4000, 5000};
    for (uint32_t id : runs) {
        ids[windowFileSlotToWrite(ids)] = id;
    }
    EXPECT_EQ(ids[0], 4000u);
    EXPECT_EQ(ids[1], 5000u);
    EXPECT_EQ(ids[2], 3000u);
}

TEST(TrailVo2Link, TheCorrectedSpeedIsWhatTheWindowsCarry)
{
    // The feed hands the builder the filter's speed; a 4% correction shows in
    // the window's mean speed (Run R6b: the raw speed reads low).
    WindowBuilder b;
    WindowRecord  w;
    for (int i = 0; i < 60; ++i) {
        b.addSecond(steady(3.0f * 1.04f));
    }
    ASSERT_TRUE(b.popWindow(w));
    EXPECT_EQ(w.speedMMinX10, 1872);   // 3.12 m/s = 187.2 m/min
}

TEST(TrailVo2Link, ARunWithNoSteadyMinutesStillLeavesItsMaxHr)
{
    // Short run: no windows, but a sustained max HR is worth leaving for Run's
    // auto max (Service::saveVo2Windows writes when either exists).
    WindowBuilder b;
    for (int i = 0; i < 400; ++i) {
        b.addSecond(steady(1.0f, 170));   // walking pace: no good seconds
    }
    EXPECT_EQ(b.sustainedMaxHr(), 170);
    char buf[kWindowFileBytes];
    const size_t len = writeWindowFile(buf, sizeof(buf), 77u, b.sustainedMaxHr(), nullptr, 0);
    ASSERT_GT(len, 0u);
    WindowFileReader rd(buf, len);
    WindowFileInfo   info;
    ASSERT_TRUE(rd.open(info));
    EXPECT_EQ(info.count, 0);
    EXPECT_EQ(info.maxHr, 170);
}
