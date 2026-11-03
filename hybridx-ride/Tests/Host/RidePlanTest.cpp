// HybridX Ride: indoor interval plans (RidePlan.hpp, PlanRunner.hpp, RidePlanText.hpp)
// and the distance auto lap (RideModel.hpp).

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "RidePlanText.hpp"

using namespace Ride;

namespace {

constexpr uint32_t kStarts[] = {0u, 0xFFFFFFFFu - 100000u};   // either side of the clock's wrap

PlanConfig of(SessionType t)
{
    PlanConfig c;
    c.type = t;
    return c;
}

struct Fixture {
    RideModel m;
    PlanRunner r;
    uint32_t t0 = 0;
    std::vector<Cue> cues;
    void start(const PlanConfig &p, uint32_t s0 = 0, uint32_t autoLapMs = 0, uint32_t autoLapDistCm = 0,
               bool outdoor = false)
    {
        t0 = s0;
        RideConfig c;
        c.outdoor       = outdoor;
        c.autoLapMs     = autoLapMs;
        c.autoLapDistCm = autoLapDistCm;
        r.start(p, m, t0, c);
    }
    void run(uint32_t fromMs, uint32_t toMs, uint16_t hr = 130)
    {
        for (uint32_t off = fromMs; off <= toMs; off += 1000) {
            m.addSample(t0 + off, hr);
            const Cue c = r.tick(m, t0 + off);
            if (c != Cue::None) {
                cues.push_back(c);
            }
        }
    }
    int count(Cue c) const
    {
        int n = 0;
        for (Cue x : cues) {
            n += x == c ? 1 : 0;
        }
        return n;
    }
};

template <typename F>
std::string text(F f)
{
    char buf[40];
    f(buf, sizeof(buf));
    return buf;
}

} // namespace

TEST(RidePlan, TheKindsAndTheirSteps)
{
    EXPECT_EQ(stepCount(of(SessionType::Plain)), 0);
    EXPECT_EQ(stepCount(of(SessionType::Custom)), 11);    // 6 x (40 + 20), no last rest
    EXPECT_EQ(stepCount(of(SessionType::Tabata)), 15);
    EXPECT_EQ(plannedMs(of(SessionType::Tabata)), 230000u);
    EXPECT_EQ(plannedMs(of(SessionType::Custom)), 6u * 40000u + 5u * 20000u);
    EXPECT_EQ(stepAt(of(SessionType::Custom), 1).kind, StepKind::Rest);
    EXPECT_EQ(stepAt(of(SessionType::Custom), 10).round, 6);
    EXPECT_EQ(kindName(SessionType::Custom), std::string("Intervals"));
    EXPECT_EQ(nextKind(nextKind(nextKind(SessionType::Plain))), SessionType::Plain);
}

TEST(RidePlan, IntervalsRunOnTheClockAndIgnoreTheButton)
{
    for (uint32_t s0 : kStarts) {
        Fixture f;
        PlanConfig p = of(SessionType::Custom);
        p.workSec = 30;
        p.restSec = 15;
        p.rounds  = 3;
        f.start(p, s0);
        EXPECT_EQ(f.r.press(f.m, s0 + 10000), Cue::None);   // the timer ignores R2
        f.run(0, 130000);
        EXPECT_EQ(f.count(Cue::Rest), 2);
        EXPECT_EQ(f.count(Cue::Work), 2);
        EXPECT_EQ(f.count(Cue::Done), 1);
        EXPECT_EQ(f.count(Cue::Warning), 5);
        ASSERT_EQ(f.m.lapCount(), 5);
        const uint32_t want[] = {30000, 15000, 30000, 15000, 30000};
        for (uint8_t i = 0; i < 5; ++i) {
            EXPECT_EQ(f.m.lapAt(i).activeMs, want[i]) << int(i);
            EXPECT_EQ(f.m.lapAt(i).kind, uint8_t(i % 2 ? StepKind::Rest : StepKind::Work)) << int(i);
            EXPECT_EQ(f.m.lapAt(i).round, i / 2 + 1) << int(i);
            EXPECT_TRUE(f.m.lapAt(i).autoEnded) << int(i);   // every step ends by time
        }
        EXPECT_EQ(f.m.state(), RunState::Paused);   // the end pauses the ride
        EXPECT_TRUE(f.r.done());
    }
}

TEST(RidePlan, APlanSwitchesBothAutoLapsOff)
{
    Fixture f;
    f.start(of(SessionType::Tabata), 0, 60000, 100000);
    EXPECT_EQ(f.m.config().autoLapMs, 0u);
    EXPECT_EQ(f.m.config().autoLapDistCm, 0u);
}

TEST(RidePlan, AFreeRideLapsOnTheButton)
{
    Fixture f;
    f.start(of(SessionType::Plain));
    f.run(0, 10000);
    EXPECT_EQ(f.r.press(f.m, 10500), Cue::Round);
    EXPECT_EQ(f.m.lapCount(), 2);
    EXPECT_FALSE(f.r.hasPlan());
}

TEST(RideModel, TheDistanceAutoLapEndsALapOnItsDistance)
{
    RideModel m;
    RideConfig c;
    c.outdoor       = true;
    c.autoLapDistCm = 100000;   // 1 km
    m.start(0, c);
    Motion mo;
    mo.distCm   = 700;           // 7 m a second: 25.2 km/h
    mo.speedMmS = 7000;
    int laps = 0;
    for (uint32_t t = 1; t <= 300; ++t) {
        laps += m.addSample(t * 1000, 120, mo) ? 1 : 0;
    }
    EXPECT_EQ(laps, 2);                         // 2100 m: two whole kilometres
    ASSERT_EQ(m.lapCount(), 3);
    EXPECT_TRUE(m.lapAt(0).byDistance);
    EXPECT_TRUE(m.lapAt(0).autoEnded);
    EXPECT_EQ(m.lapAt(0).distCm, 100100u);      // 143 samples of 7 m: the one that went past stays
    EXPECT_FALSE(m.lapAt(2).byDistance);
}

TEST(RideModel, TheDistanceAutoLapIsOutdoorsOnly)
{
    RideModel m;
    RideConfig c;
    c.outdoor       = false;
    c.autoLapDistCm = 100000;
    m.start(0, c);
    Motion mo;
    mo.distCm = 700;
    for (uint32_t t = 1; t <= 300; ++t) {
        EXPECT_FALSE(m.addSample(t * 1000, 120, mo));
    }
    EXPECT_EQ(m.lapCount(), 1);
}

TEST(RidePlanText, TheWords)
{
    PlanConfig p = of(SessionType::Custom);
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, p); }), "40 s / 20 s x 6");
    p.type = SessionType::Tabata;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, p); }), "8 x 20 s / 10 s");
    p.type = SessionType::Plain;
    EXPECT_EQ(text([&](char *b, size_t n) { formatPlanSummary(b, n, p); }), "");

    EXPECT_EQ(text([](char *b, size_t n) { formatStepName(b, n, 1, 1, 3, 6, false); }), "Rest 3");
    EXPECT_EQ(text([](char *b, size_t n) { formatStepName(b, n, 0, 0, 0, 6, true); }), "Auto 6");

    PlanView v;
    v.type = 1; v.steps = 11; v.kind = 0; v.round = 3; v.rounds = 6;
    EXPECT_EQ(text([&](char *b, size_t n) { formatStepHeading(b, n, v); }), "Work 3/6");
    v.done = 1;
    EXPECT_EQ(text([&](char *b, size_t n) { formatStepHeading(b, n, v); }), "Done");

    LapEventData e;
    e.cue = uint8_t(Cue::Work); e.round = 3;
    EXPECT_EQ(text([&](char *b, size_t n) { formatCueHeading(b, n, e); }), "Work 4");
    e.cue = uint8_t(Cue::Round); e.finishedLap = 4; e.autoEnded = 0;
    EXPECT_EQ(text([&](char *b, size_t n) { formatCueHeading(b, n, e); }), "Lap 5");

    EXPECT_EQ(text([](char *b, size_t n) { formatSummaryLine(b, n, 1, 11); }), "6 rounds");
    EXPECT_EQ(text([](char *b, size_t n) { formatSummaryLine(b, n, 2, 15); }), "8 of 8 rounds");
    EXPECT_EQ(text([](char *b, size_t n) { formatSummaryLine(b, n, 0, 1); }), "1 lap");
    EXPECT_EQ(text([](char *b, size_t n) { formatCountdown(b, n, 1); }), "00:01");

    PlanConfig q = of(SessionType::Custom);
    q = stepField(q, PlanField::Work);
    EXPECT_EQ(q.workSec, 45);
    q.rounds = 30;
    q = stepField(q, PlanField::Rounds);
    EXPECT_EQ(q.rounds, 1);
    PlanField fs[3];
    EXPECT_EQ(planFields(SessionType::Tabata, fs), 0);
    EXPECT_EQ(planFields(SessionType::Custom, fs), 3);
}
