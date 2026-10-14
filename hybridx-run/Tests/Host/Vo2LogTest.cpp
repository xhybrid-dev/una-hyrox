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
    EXPECT_STREQ(buf, "window,active_s,good_s,outcome,speed_m_min,spread_pct,grade_pct,hr_avg,hr_min,hr_max,cost\n");

    WindowLog w;
    w.index = 7; w.activeSec = 420; w.goodSec = 58; w.outcome = WindowOutcome::Kept;
    w.speedMMinX10 = 2014; w.spreadX10 = 21; w.gradeX10 = -15; w.hrX10 = 1482;
    w.hrMin = 141; w.hrMax = 153; w.costX10 = 438;
    ASSERT_GT(Log::windowLine(w, buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "7,420,58,kept,201.4,2.1,-1.5,148.2,141,153,43.8\n");

    RunResult r;
    r.status = RunStatus::Ok; r.vo2x10 = 544; r.windowsUsed = 25; r.tooEasy = 1;
    ASSERT_GT(Log::resultLine(r, profile(), 186, buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "# result ok (profile ok), vo2max 54.4, windows 25, too_easy 1, above_max 0, "
                      "implausible 0, max_hr 180 (age), rest_hr 52 (watch), run_max_hr 186\n");
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
    w.hrMin = 255; w.hrMax = 255; w.costX10 = 65535;
    EXPECT_GT(Log::windowLine(w, buf, sizeof(buf)), 0u);
    RunResult r;
    r.status = RunStatus::ProfileIncomplete; r.profileStatus = ProfileStatus::ReserveTooSmall;
    r.vo2x10 = 65535; r.windowsUsed = 65535; r.tooEasy = 65535; r.aboveMax = 65535; r.implausible = 65535;
    EXPECT_GT(Log::resultLine(r, p, 255, buf, sizeof(buf)), 0u);
}
