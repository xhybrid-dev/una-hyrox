#include <gtest/gtest.h>

#include <cstring>

#include "FrameWatch.hpp"
#include "WristLog.hpp"
#include "WristMode.hpp"

using namespace ClockProbe;

// ---- WristLog ---------------------------------------------------------------------------------

TEST(WristLog, StartsEmptyAndHasNoAge)
{
    WristLog w;
    EXPECT_FALSE(w.any());
    EXPECT_EQ(w.today(), 0u);
    EXPECT_EQ(w.lastAgeMs(1000), 0xFFFFFFFFu);
    EXPECT_EQ(w.within(1000, 5000), 0);
}

TEST(WristLog, CountsAndAgesTheLastRaise)
{
    WristLog w;
    w.record(1000);
    w.record(4000);
    EXPECT_TRUE(w.any());
    EXPECT_EQ(w.today(), 2u);
    EXPECT_EQ(w.total(), 2u);
    EXPECT_EQ(w.lastAgeMs(9000), 5000u);
    EXPECT_EQ(w.within(9000, 6000), 1);    // only the one at 4000 is within 6 s
    EXPECT_EQ(w.within(9000, 9000), 2);
}

TEST(WristLog, ANewDayResetsTodayButNotTheRing)
{
    WristLog w;
    w.record(10);
    w.newDay();
    EXPECT_EQ(w.today(), 0u);
    EXPECT_EQ(w.total(), 1u);
    EXPECT_EQ(w.lastAgeMs(60), 50u);
}

TEST(WristLog, TheRingKeepsOnlyTheLastSixteenButTheCountsKeepGoing)
{
    WristLog w;
    for (uint32_t i = 0; i < 40; ++i) {
        w.record(i * 100);
    }
    EXPECT_EQ(w.total(), 40u);
    EXPECT_EQ(w.within(4000, 100000), WristLog::kKeep);
    EXPECT_EQ(w.lastAgeMs(4000), 100u);   // the last was at 3900
}

TEST(WristLog, TheTickWrapIsHarmless)
{
    WristLog w;
    w.record(0xFFFFFF00u);
    EXPECT_EQ(w.lastAgeMs(0x00000100u), 0x200u);   // 512 ms later, across the wrap at about 49 days
    EXPECT_EQ(w.within(0x00000100u, 1000), 1);
}

// ---- FrameWatch -------------------------------------------------------------------------------

TEST(FrameWatch, TenFramesASecondReadsAsOneHundredTenths)
{
    FrameWatch f;
    f.onReport(10000, 500, 1, 0);
    EXPECT_FALSE(f.haveRate());   // one report is no rate
    f.onReport(15000, 550, 1, 0);
    EXPECT_TRUE(f.haveRate());
    EXPECT_EQ(f.fpsX10(), 100);
    f.onReport(20000, 600, 2, 1);
    EXPECT_EQ(f.fpsX10(), 100);
    EXPECT_EQ(f.resumes(), 2);
    EXPECT_EQ(f.suspends(), 1);
}

TEST(FrameWatch, ASlowerTickReadsSlower)
{
    FrameWatch f;
    f.onReport(0, 0, 0, 0);
    f.onReport(5000, 25, 0, 0);   // 5 frames a second
    EXPECT_EQ(f.fpsX10(), 50);
}

TEST(FrameWatch, TheLongestGapInAMinuteIsKept)
{
    FrameWatch f;
    f.onReport(0, 0, 0, 0);
    f.onReport(5000, 50, 0, 0);
    f.onReport(65000, 100, 0, 0);   // a minute of nothing between: the screen was off
    EXPECT_EQ(f.maxGapMs(), 60000u);
    EXPECT_EQ(f.reportsThisMinute(), 3);
    f.startMinute();
    EXPECT_EQ(f.maxGapMs(), 0u);
    EXPECT_EQ(f.reportsThisMinute(), 0);
    EXPECT_TRUE(f.have());
}

TEST(FrameWatch, ARestartedGuiIsNotAnImpossibleRate)
{
    FrameWatch f;
    f.onReport(0, 0, 0, 0);
    f.onReport(5000, 50, 0, 0);
    f.onReport(10000, 3, 0, 0);   // its counter began again
    EXPECT_FALSE(f.haveRate());
}

TEST(FrameWatch, TimeSinceTheLastReportWrapsSafely)
{
    FrameWatch f;
    EXPECT_EQ(f.sinceReportMs(5), 0xFFFFFFFFu);
    f.onReport(0xFFFFFF00u, 0, 0, 0);
    EXPECT_EQ(f.sinceReportMs(0x100u), 0x200u);
}

// ---- WristMode --------------------------------------------------------------------------------

TEST(WristMode, AutoAlternatesByDay)
{
    EXPECT_TRUE(wristOn(ModeSetting::Auto, 20600));
    EXPECT_FALSE(wristOn(ModeSetting::Auto, 20601));
    EXPECT_TRUE(wristOn(ModeSetting::Auto, 20602));
}

TEST(WristMode, AnOverridePinsIt)
{
    EXPECT_TRUE(wristOn(ModeSetting::On, 20601));
    EXPECT_FALSE(wristOn(ModeSetting::Off, 20600));
}

TEST(WristMode, ReadsModeTxtLeniently)
{
    auto p = [](const char* s) { return parseModeText(s, std::strlen(s)); };
    EXPECT_EQ(p("on"), ModeSetting::On);
    EXPECT_EQ(p("  ON\r\n"), ModeSetting::On);
    EXPECT_EQ(p("Off # no wrist today"), ModeSetting::Off);
    EXPECT_EQ(p("auto"), ModeSetting::Auto);
    EXPECT_EQ(p(""), ModeSetting::Auto);
    EXPECT_EQ(p("only"), ModeSetting::Auto);     // "on" must be a whole word
    EXPECT_EQ(p("office"), ModeSetting::Auto);
    EXPECT_EQ(parseModeText(nullptr, 0), ModeSetting::Auto);
    EXPECT_STREQ(modeName(ModeSetting::Off), "off");
}
