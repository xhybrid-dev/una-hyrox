// HybridX Run: scoring the kept runs again (Vo2Rescore), and the history edits it uses.

#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <vector>

#include "Vo2Rescore.hpp"
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

WindowRecord window(uint32_t activeSec, uint16_t hrX10 = 1600)
{
    WindowRecord w;
    w.activeSec    = activeSec;
    w.goodSec      = 60;
    w.speedMMinX10 = 2000;   // 200 m/min
    w.spreadX10    = 20;
    w.hrX10        = hrX10;
    w.hrMin        = static_cast<uint8_t>(hrX10 / 10 - 2);
    w.hrMax        = static_cast<uint8_t>(hrX10 / 10 + 2);
    return w;
}

/// A run's file: the warm-up's five minutes and then @p used steady ones, back to back.
std::string runFile(uint16_t used, uint16_t hrX10 = 1600)
{
    std::vector<WindowRecord> ws;
    for (uint16_t i = 1; i <= 5 + used; ++i) {
        ws.push_back(window(60u * i, hrX10));
    }
    std::unique_ptr<char[]> buf(new char[kWindowFileBytes]);
    const size_t len = writeWindowFile(buf.get(), kWindowFileBytes, 1793000000u, 0, ws.data(),
                                       static_cast<uint16_t>(ws.size()));
    EXPECT_GT(len, 0u);
    return std::string(buf.get(), len);
}

}  // namespace

TEST(Vo2Rescore, ARunScoresAgainWithTheProfileOfToday)
{
    const std::string f = runFile(20);
    Vo2Run run;
    uint16_t vo2 = 0, win = 0;
    ASSERT_EQ(rescoreRun(f.data(), f.size(), false, profile(190, 50), run, vo2, win), RescoreOutcome::Scored);
    EXPECT_EQ(vo2, 544);   // the 54.4 reference (SteadyRunMatchesTheEquations)
    EXPECT_EQ(win, 20);
    // The same minutes with a higher max HR read higher: the same heart rate is a smaller share of
    // a bigger reserve, so the pace it held implies a larger capacity.
    uint16_t vo2b = 0;
    ASSERT_EQ(rescoreRun(f.data(), f.size(), false, profile(200, 50), run, vo2b, win), RescoreOutcome::Scored);
    EXPECT_EQ(vo2b, 580);
}

TEST(Vo2Rescore, NoReadableFileKeepsTheRun)
{
    Vo2Run run;
    uint16_t vo2 = 1, win = 1;
    EXPECT_EQ(rescoreRun(nullptr, 0, false, profile(), run, vo2, win), RescoreOutcome::NoFile);
    EXPECT_EQ(rescoreRun("{nonsense", 9, false, profile(), run, vo2, win), RescoreOutcome::NoFile);
    EXPECT_EQ(vo2, 0);
}

TEST(Vo2Rescore, TooFewMinutesLeaveNoFigure)
{
    const std::string f = runFile(2);   // under the minimum windows
    Vo2Run run;
    uint16_t vo2 = 0, win = 0;
    EXPECT_EQ(rescoreRun(f.data(), f.size(), false, profile(), run, vo2, win), RescoreOutcome::NoFigure);
}

TEST(Vo2Rescore, TheScratchRunIsLeftForALiveRun)
{
    // A re-score must not leave Trail's known-only judging, or its windows, in the run object.
    const std::string f = runFile(10);
    Vo2Run run;
    uint16_t vo2 = 0, win = 0;
    rescoreRun(f.data(), f.size(), true, profile(), run, vo2, win);
    EXPECT_EQ(run.counts().accepted, 0);
    EXPECT_EQ(run.addWindow(window(600)), WindowOutcome::Lagged);   // live rules: nothing before
}

TEST(Vo2Rescore, TheProfileKeyFollowsWhatTheAthleteEntered)
{
    ProfileInput a;
    a.birthYear = 1986;
    a.birthMonth = 5;
    const uint32_t k = profileKey(a);
    EXPECT_NE(k, 0u);
    ProfileInput b = a;
    b.watchRestHr = 48;   // the watch's figure changes daily: not a reason to score again
    b.autoMaxHr   = 185;
    EXPECT_EQ(profileKey(b), k);
    b.enteredMaxHr = 186;
    EXPECT_NE(profileKey(b), k);
    ProfileInput c = a;
    c.birthMonth = 6;
    EXPECT_NE(profileKey(c), k);
}

TEST(Vo2Rescore, RunFilesAreNamedByTheirTime)
{
    char p[32];
    EXPECT_EQ(runFilePath(p, sizeof(p), 1793000000u), std::strlen("Runs/1793000000.json"));
    EXPECT_STREQ(p, "Runs/1793000000.json");
    char tiny[8];
    EXPECT_EQ(runFilePath(tiny, sizeof(tiny), 1793000000u), 0u);
    EXPECT_STREQ(tiny, "");
}

TEST(Vo2History, RescoringEditsKeepTheTrendInStep)
{
    const uint32_t day = 86400u;
    Vo2History h;
    h.add(RunRecord{1793000000u, 500, 30});
    h.add(RunRecord{1793000000u + day, 520, 30});
    h.add(RunRecord{1793000000u + 2 * day, 540, 30});
    h.setRun(1, 460, 30);
    h.removeAt(2);
    h.recomputeShown();
    ASSERT_EQ(h.size(), 2);
    EXPECT_EQ(h.at(1).vo2x10, 460);
    EXPECT_EQ(h.at(0).shownX10, 500);
    EXPECT_EQ(h.at(1).shownX10, 480);
    EXPECT_EQ(h.rollingX10(), 480);
    h.setRun(5, 1, 1);   // out of range: nothing
    h.removeAt(9);
    EXPECT_EQ(h.size(), 2);
}

TEST(Vo2History, TheProfileKeyIsSavedAndAnOlderFileHasNone)
{
    Vo2History h;
    h.add(RunRecord{1793000000u, 500, 30});
    h.setProfileKey(123456u);
    char buf[Vo2History::kMaxJsonBytes];
    const size_t len = h.toJson(buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    Vo2History r;
    ASSERT_TRUE(r.fromJson(buf, len));
    EXPECT_EQ(r.profileKey(), 123456u);

    const char* old = R"({"v":4,"restHr":52,"trail":0,"maxes":[],"runs":[[1793000000,500,30,0,500]]})";
    ASSERT_TRUE(r.fromJson(old, std::strlen(old)));
    EXPECT_EQ(r.profileKey(), 0u);   // never scored with a key: scored once on the next open
}
