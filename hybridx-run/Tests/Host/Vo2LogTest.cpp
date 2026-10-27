#include <gtest/gtest.h>

#include <cstring>

#include "Vo2Log.hpp"

using namespace RunVo2;

namespace
{
Profile profile()
{
    Profile p;
    p.status     = ProfileStatus::Ok;
    p.ageYears   = 40;
    p.maxHr      = 180;
    p.maxSource  = MaxHrSource::Formula;
    p.restHr     = 52;
    p.restSource = RestHrSource::Watch;
    return p;
}
}  // namespace

TEST(Vo2Log, Lines)
{
    char buf[Log::kMaxLineBytes];
    ASSERT_GT(Log::contextLine(1791963000, profile(), buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "# HybridX Run VO2 log 1, start 2026-10-14T07:30:00Z (1791963000), "
                      "max_hr 180 (age), rest_hr 52 (watch), age 40, profile ok\n");

    ASSERT_GT(Log::headerLine(buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "window,active_s,good_s,outcome,speed_m_min,spread_pct,grade_pct,hr_avg,hr_min,hr_max,cost,scale,lag\n");

    WindowLog w;
    w.index = 7; w.activeSec = 420; w.goodSec = 58; w.outcome = WindowOutcome::Kept;
    w.speedMMinX10 = 2014; w.spreadX10 = 21; w.gradeX10 = -15; w.hrX10 = 1482;
    w.hrMin = 141; w.hrMax = 153; w.costX10 = 438; w.speedScaleX1000 = 1034;
    ASSERT_GT(Log::windowLine(w, buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "7,420,58,kept,201.4,2.1,-1.5,148.2,141,153,43.8,1.034,0\n");
    w.afterChange = true;   // P3: the minute before was at another pace
    w.outcome     = WindowOutcome::Lagged;
    ASSERT_GT(Log::windowLine(w, buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "7,420,58,lagged,201.4,2.1,-1.5,148.2,141,153,43.8,1.034,1\n");

    RunResult r;
    r.status = RunStatus::Ok; r.vo2x10 = 544; r.windowsUsed = 25; r.tooEasy = 1;
    ASSERT_GT(Log::resultLine(r, profile(), 186, buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "# result ok (profile ok), vo2max 54.4, windows 25, too_easy 1, above_max 0, "
                      "implausible 0, max_hr 180 (age), rest_hr 52 (watch), run_max_hr 186\n");

    // P3: what the guards would have made of it, and the shown value with the age limit.
    r.afterChange = 3; r.withoutLagX10 = 541; r.firstHalfX10 = 548; r.secondHalfX10 = 539; r.spreadX10 = 12;
    ASSERT_GT(Log::whatIfLine(r, 532, 529, buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "# what-if (not used yet) after_change 3, without_lag 54.1, first_half 54.8, second_half 53.9, "
                      "spread 1.2, shown 53.2, shown_42d 52.9\n");
    // Too few windows for a figure reads as 0.0, and no recent runs as 0.0.
    RunResult none;
    ASSERT_GT(Log::whatIfLine(none, 0, 0, buf, sizeof(buf)), 0u);
    EXPECT_NE(std::strstr(buf, "without_lag 0.0, first_half 0.0"), nullptr);
}

TEST(Vo2Log, UnknownSourcesAndTooSmallBuffers)
{
    Profile p;   // nothing known
    char buf[Log::kMaxLineBytes];
    ASSERT_GT(Log::contextLine(0, p, buf, sizeof(buf)), 0u);
    EXPECT_NE(std::strstr(buf, "max_hr 0 (none), rest_hr 0 (none)"), nullptr);
    EXPECT_NE(std::strstr(buf, "profile needs_age"), nullptr);

    char tiny[16];
    EXPECT_EQ(Log::headerLine(tiny, sizeof(tiny)), 0u);
    EXPECT_STREQ(tiny, "");
    WindowLog w;
    EXPECT_EQ(Log::windowLine(w, tiny, sizeof(tiny)), 0u);
    EXPECT_EQ(Log::windowLine(w, nullptr, 0), 0u);
}

TEST(Vo2Log, WorstCaseLinesFit)
{
    // The longest values every field can hold still fit kMaxLineBytes.
    Profile p = profile();
    p.status = ProfileStatus::ReserveTooSmall;
    p.maxHr = 255; p.restHr = 255; p.ageYears = 255;
    char buf[Log::kMaxLineBytes];
    EXPECT_GT(Log::contextLine(0xFFFFFFFFll, p, buf, sizeof(buf)), 0u);
    WindowLog w;
    w.index = 65535; w.activeSec = 0xFFFFFFFFu; w.goodSec = 65535; w.outcome = WindowOutcome::Overflow;
    w.speedMMinX10 = 65535; w.spreadX10 = 65535; w.gradeX10 = -32768; w.hrX10 = 65535;
    w.hrMin = 255; w.hrMax = 255; w.costX10 = 65535; w.speedScaleX1000 = 65535;
    EXPECT_GT(Log::windowLine(w, buf, sizeof(buf)), 0u);
    RunResult r;
    r.status = RunStatus::ProfileIncomplete; r.profileStatus = ProfileStatus::ReserveTooSmall;
    r.vo2x10 = 65535; r.windowsUsed = 65535; r.tooEasy = 65535; r.aboveMax = 65535; r.implausible = 65535;
    EXPECT_GT(Log::resultLine(r, p, 255, buf, sizeof(buf)), 0u);
    r.afterChange = 65535; r.withoutLagX10 = 65535; r.firstHalfX10 = 65535; r.secondHalfX10 = 65535;
    r.spreadX10 = 65535;
    EXPECT_GT(Log::whatIfLine(r, 65535, 65535, buf, sizeof(buf)), 0u);
    w.afterChange = true;
    w.outcome     = WindowOutcome::Lagged;
    EXPECT_GT(Log::windowLine(w, buf, sizeof(buf)), 0u);
}
