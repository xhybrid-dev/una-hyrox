/**
 * The run detail page's numbers: zones, best splits, halves and drift.
 */

#include <gtest/gtest.h>

#include <cmath>
#include <cstring>

#include "RunStats.hpp"
#include "RunStatsText.hpp"
#include "Vo2Window.hpp"

using namespace RunVo2;

namespace
{

const uint8_t kZones[6] = {95, 114, 133, 152, 171, 190};   // the watch's defaults

Sample running(float speedMs, uint8_t hr, float gradePct = 0.0f, bool gradeValid = false)
{
    Sample s;
    s.active     = true;
    s.speedMs    = speedMs;
    s.speedValid = true;
    s.gradePct   = gradePct;
    s.gradeValid = gradeValid;
    s.hrBpm      = hr;
    s.hrTrust    = 2.0f;
    return s;
}

/// Feed @p seconds of one speed and heart rate, adding the distance to @p dist.
void feed(RunStats& st, float& dist, unsigned seconds, float speedMs, uint8_t hr, float gradePct = 0.0f,
          bool gradeValid = false)
{
    for (unsigned i = 0; i < seconds; ++i) {
        dist += speedMs;
        st.addSecond(running(speedMs, hr, gradePct, gradeValid), dist);
    }
}

void pause(RunStats& st, float dist, unsigned seconds)
{
    Sample p;
    p.active = false;
    for (unsigned i = 0; i < seconds; ++i) {
        st.addSecond(p, dist);
    }
}

uint32_t zoneTotal(const RunStatsResult& r)
{
    uint32_t t = 0;
    for (const uint32_t z : r.zoneSec) {
        t += z;
    }
    return t;
}

}  // namespace

// ---- zones ------------------------------------------------------------------------------

TEST(RunStatsZones, ASecondFallsInTheZoneTheArcWouldShow)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 10, 3.0f, 90);    // below 95: below zone 1
    feed(st, d, 20, 3.0f, 100);   // zone 1 (above 95)
    feed(st, d, 30, 3.0f, 120);   // zone 2
    feed(st, d, 40, 3.0f, 140);   // zone 3
    feed(st, d, 50, 3.0f, 160);   // zone 4
    feed(st, d, 60, 3.0f, 180);   // zone 5
    const RunStatsResult r = st.result();
    EXPECT_EQ(r.zoneCount, 5);
    EXPECT_EQ(r.zoneSec[0], 10u);
    EXPECT_EQ(r.zoneSec[1], 20u);
    EXPECT_EQ(r.zoneSec[2], 30u);
    EXPECT_EQ(r.zoneSec[3], 40u);
    EXPECT_EQ(r.zoneSec[4], 50u);
    EXPECT_EQ(r.zoneSec[5], 60u);
}

TEST(RunStatsZones, AThresholdItselfIsTheZoneBelow)
{
    // "Above" the threshold enters the zone, as HeartRateZone::setHR does: 95 is still below zone 1.
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 5, 3.0f, 95);
    feed(st, d, 5, 3.0f, 96);
    feed(st, d, 5, 3.0f, 171);   // not above zone 5's entry
    feed(st, d, 5, 3.0f, 172);
    const RunStatsResult r = st.result();
    EXPECT_EQ(r.zoneSec[0], 5u);
    EXPECT_EQ(r.zoneSec[1], 5u);
    EXPECT_EQ(r.zoneSec[4], 5u);   // 171 is in zone 4 (above 152)
    EXPECT_EQ(r.zoneSec[5], 5u);
}

TEST(RunStatsZones, AboveTheTopThresholdStaysInZoneFive)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 30, 3.0f, 205);
    EXPECT_EQ(st.result().zoneSec[5], 30u);
}

TEST(RunStatsZones, OnlyGoodHeartRateSecondsAreCountedAndAPauseIsNot)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 20, 3.0f, 140);
    Sample bad = running(3.0f, 140);
    bad.hrTrust = 0.0f;   // the kernel does not trust it
    for (int i = 0; i < 15; ++i) {
        d += 3.0f;
        st.addSecond(bad, d);
    }
    feed(st, d, 5, 3.0f, 0);   // no heart rate
    pause(st, d, 50);
    const RunStatsResult r = st.result();
    EXPECT_EQ(r.zoneSec[3], 20u);
    EXPECT_EQ(zoneTotal(r), 20u);
    EXPECT_EQ(r.activeSec, 40u);   // the pause is not active time
}

TEST(RunStatsZones, NoThresholdsMeansNoZones)
{
    RunStats st;
    st.reset();
    float d = 0.0f;
    feed(st, d, 30, 3.0f, 140);
    const RunStatsResult r = st.result();
    EXPECT_EQ(r.zoneCount, 0);
    EXPECT_EQ(zoneTotal(r), 0u);
}

// ---- best splits --------------------------------------------------------------------------

TEST(RunStatsSplits, ASteadyRunHasEveryBestAtItsPace)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 1500, 4.0f, 150);   // 6 km at 4 m/s
    const RunStatsResult r = st.result();
    EXPECT_NEAR(r.bestMs[0], 250000.0, 3.0);               // 1 km
    EXPECT_NEAR(r.bestMs[1], 1609.344 / 4.0 * 1000.0, 3.0);   // 1 mile
    EXPECT_NEAR(r.bestMs[2], 1250000.0, 3.0);              // 5 km
}

TEST(RunStatsSplits, ADistanceNotYetRunIsNone)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 300, 4.0f, 150);   // 1.2 km
    const RunStatsResult r = st.result();
    EXPECT_GT(r.bestMs[0], 0u);
    EXPECT_EQ(r.bestMs[1], 0u);
    EXPECT_EQ(r.bestMs[2], 0u);
}

TEST(RunStatsSplits, TheFastestKilometreInsideASlowerRunIsFound)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 334, 3.0f, 150);   // 1,002 m
    feed(st, d, 200, 5.0f, 170);   // 1,000 m fast: 200 s
    feed(st, d, 400, 3.0f, 150);   // 1,200 m
    const RunStatsResult r = st.result();
    // The best ends on a 10 m boundary, so it can include a little of the slow part either side.
    EXPECT_GE(r.bestMs[0], 199900u);
    EXPECT_LE(r.bestMs[0], 201000u);
}

TEST(RunStatsSplits, APauseIsNotPartOfASplit)
{
    RunStats a, b;
    a.reset(kZones, 6);
    b.reset(kZones, 6);
    float da = 0.0f, db = 0.0f;
    feed(a, da, 1000, 4.0f, 150);
    feed(b, db, 500, 4.0f, 150);
    pause(b, db, 600);
    feed(b, db, 500, 4.0f, 150);
    EXPECT_EQ(a.result().bestMs[0], b.result().bestMs[0]);
    EXPECT_EQ(a.result().bestMs[2], b.result().bestMs[2]);
}

TEST(RunStatsSplits, AJumpInDistanceIsNotASplit)
{
    // 900 m, then 500 m in one second (a GPS catch-up), then on at the same pace. A split that
    // spanned the jump would be far quicker than anyone can run.
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 300, 3.0f, 150);
    d += 500.0f;
    st.addSecond(running(3.0f, 150), d);
    feed(st, d, 400, 3.0f, 150);
    const RunStatsResult r = st.result();
    EXPECT_GE(r.bestMs[0], 330000u);   // 1 km at 3 m/s is 333 s
    EXPECT_LE(r.bestMs[0], 336000u);
}

TEST(RunStatsSplits, ADistanceThatGoesBackwardsRestartsTheSplits)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 400, 4.0f, 150);   // 1.6 km: a best exists
    const uint32_t before = st.result().bestMs[0];
    ASSERT_GT(before, 0u);
    d = 0.0f;   // the counter was reset
    st.addSecond(running(4.0f, 150), d);
    feed(st, d, 100, 2.0f, 150);   // 200 m more: too short for a new 1 km
    EXPECT_EQ(st.result().bestMs[0], before);   // the earlier best stands
}

TEST(RunStatsSplits, TheFiveKilometreLookbackFitsTheRing)
{
    // A long run, well past the ring's length, still finds its 5 km best.
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 3000, 3.0f, 150);   // 9 km at 3 m/s
    feed(st, d, 1500, 5.0f, 170);   // 7.5 km at 5 m/s
    EXPECT_NEAR(st.result().bestMs[2], 1000000.0, 3000.0);
}

// ---- halves -------------------------------------------------------------------------------

TEST(RunStatsHalves, ASteadyRunHasEqualHalves)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 1000, 3.0f, 150);
    const RunStatsResult r = st.result();
    EXPECT_NEAR(r.halfPace[0], 1.0 / 3.0, 0.002);
    EXPECT_NEAR(r.halfPace[1], 1.0 / 3.0, 0.002);
    EXPECT_EQ(r.halfHr[0], 150);
    EXPECT_EQ(r.halfHr[1], 150);
}

TEST(RunStatsHalves, ANegativeSplitShowsInTheHalves)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 600, 3.0f, 140);
    feed(st, d, 600, 4.0f, 160);
    const RunStatsResult r = st.result();
    EXPECT_NEAR(r.halfPace[0], 1.0 / 3.0, 0.003);
    EXPECT_NEAR(r.halfPace[1], 1.0 / 4.0, 0.003);
    EXPECT_EQ(r.halfHr[0], 140);
    EXPECT_EQ(r.halfHr[1], 160);
}

TEST(RunStatsHalves, AVeryShortRunHasNoHalves)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 90, 3.0f, 150);
    const RunStatsResult r = st.result();
    EXPECT_EQ(r.halfPace[0], 0.0f);
    EXPECT_EQ(r.halfPace[1], 0.0f);
}

TEST(RunStatsHalves, ALongRunKeepsTheHalvesAndTheMemoryBounded)
{
    // Four hours, a change of pace at the exact half: the checkpoints have been thinned several
    // times, and the halves are still right to within their spacing (3% of the run).
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 7200, 3.0f, 140);
    feed(st, d, 7200, 4.0f, 160);
    const RunStatsResult r = st.result();
    EXPECT_NEAR(r.halfPace[0], 1.0 / 3.0, 0.03 / 3.0);
    EXPECT_NEAR(r.halfPace[1], 1.0 / 4.0, 0.03 / 4.0);
}

TEST(RunStatsHalves, ADayLongRunDoesNotOverrunAnything)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 86400, 2.0f, 130);   // 24 hours: for the sanitizers
    const RunStatsResult r = st.result();
    EXPECT_EQ(r.activeSec, 86400u);
    EXPECT_GT(r.halfPace[0], 0.0f);
}

// ---- drift ----------------------------------------------------------------------------------

TEST(RunStatsDrift, HeartRateRisingForTheSameSpeedIsDrift)
{
    // 2,400 s: the warm-up (300 s) at a lower rate, then 1,050 s at 140 and 1,050 s at 147, all at 3 m/s.
    // EF is speed / heart rate, so drift is 1 - 140/147 = 4.76%.
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 300, 3.0f, 120);
    feed(st, d, 1050, 3.0f, 140);
    feed(st, d, 1050, 3.0f, 147);
    const RunStatsResult r = st.result();
    ASSERT_EQ(r.driftStatus, DriftStatus::Ok);
    // 4.76% is 47.6. Past 1,920 s the checkpoints are 60 s apart, so the step in heart rate at 1,350 s
    // (a multiple of 30 but not of 60) falls between two of them and the interpolation smooths it:
    // the figure is 46, about 0.15 of a point low. That is the design's tolerance, not a bug.
    EXPECT_NEAR(r.driftX10, 47.6, 2.0);
    // EF: 3 m/s = 180 m/min over 140 bpm = 1.286; over 147 = 1.224 (within the same smoothing).
    EXPECT_NEAR(r.efX1000[0], 1286, 3);
    EXPECT_NEAR(r.efX1000[1], 1224, 3);

    // A step that falls on a checkpoint is exact: a shorter run keeps its 30 s checkpoints.
    RunStats exact;
    exact.reset(kZones, 6);
    float e = 0.0f;
    feed(exact, e, 300, 3.0f, 120);
    feed(exact, e, 900, 3.0f, 140);   // the post-warm-up span is 1,800 s: its middle is second 1,200
    feed(exact, e, 900, 3.0f, 147);
    const RunStatsResult x = exact.result();
    ASSERT_EQ(x.driftStatus, DriftStatus::Ok);
    EXPECT_NEAR(x.driftX10, 47.6, 0.5);
}

TEST(RunStatsDrift, ATreadmillRunHasNoneButKeepsItsOtherNumbers)
{
    // The same run as above, on a treadmill: zones, splits and halves stand; drift does not.
    RunStats st;
    st.reset(kZones, 6);
    st.setTreadmill(true);
    float d = 0.0f;
    feed(st, d, 300, 3.0f, 120);
    feed(st, d, 1050, 3.0f, 140);
    feed(st, d, 1050, 3.0f, 147);
    const RunStatsResult r = st.result();
    EXPECT_EQ(r.driftStatus, DriftStatus::Treadmill);
    EXPECT_EQ(r.driftX10, 0);
    EXPECT_GT(r.bestMs[0], 0u);
    EXPECT_GT(r.halfPace[0], 0.0f);
    EXPECT_GT(r.zoneCount, 0u);

    st.reset(kZones, 6);   // a new run is outdoors again unless told
    float e = 0.0f;
    feed(st, e, 2400, 3.0f, 140);
    EXPECT_NE(st.result().driftStatus, DriftStatus::Treadmill);
}

TEST(RunStatsDrift, AFlatRunHasNoDrift)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 300, 3.0f, 125);
    feed(st, d, 2100, 3.0f, 145);
    const RunStatsResult r = st.result();
    ASSERT_EQ(r.driftStatus, DriftStatus::Ok);
    EXPECT_EQ(r.driftX10, 0);
}

TEST(RunStatsDrift, HeartRateFallingIsNegativeDrift)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 300, 3.0f, 125);
    feed(st, d, 1050, 3.0f, 150);
    feed(st, d, 1050, 3.0f, 145);
    const RunStatsResult r = st.result();
    ASSERT_EQ(r.driftStatus, DriftStatus::Ok);
    EXPECT_LT(r.driftX10, 0);
}

TEST(RunStatsDrift, AShortRunIsGivenNone)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 300, 3.0f, 125);
    feed(st, d, 1500, 3.0f, 145);   // 1,500 s after the warm-up: under the 1,800 s needed
    EXPECT_EQ(st.result().driftStatus, DriftStatus::TooShort);
}

TEST(RunStatsDrift, WithoutAHeartRateThereIsNoDrift)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 2500, 3.0f, 0);
    EXPECT_EQ(st.result().driftStatus, DriftStatus::NoHeartRate);
}

TEST(RunStatsDrift, ARunThatChangedPaceIsNotSteady)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 300, 3.0f, 125);
    feed(st, d, 1050, 3.0f, 140);
    feed(st, d, 1050, 4.0f, 160);   // a third faster
    EXPECT_EQ(st.result().driftStatus, DriftStatus::NotSteady);
}

TEST(RunStatsDrift, HillsAreLevelledByTheGradeAdjustedSpeed)
{
    // The first half is flat; the second climbs 5% at the same pace. The climb costs more, so the
    // level-equivalent speed is higher (x 1.225 with ACSM's factor) and the climb with the same heart
    // rate looks like better efficiency, not drift: the halves differ by 22% in level speed, so the
    // run is "not steady" rather than given a misleading figure.
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 300, 3.0f, 125);
    feed(st, d, 1050, 3.0f, 145);
    feed(st, d, 1050, 3.0f, 145, 5.0f, true);
    EXPECT_EQ(st.result().driftStatus, DriftStatus::NotSteady);

    // A gentle 1% second half is within the tolerance, and the grade adjustment is applied.
    RunStats gentle;
    gentle.reset(kZones, 6);
    float g = 0.0f;
    feed(gentle, g, 300, 3.0f, 125);
    feed(gentle, g, 1050, 3.0f, 145);
    feed(gentle, g, 1050, 3.0f, 145, 1.0f, true);
    const RunStatsResult r = gentle.result();
    ASSERT_EQ(r.driftStatus, DriftStatus::Ok);
    EXPECT_LT(r.driftX10, 0);   // the same heart rate for a higher level speed
}

// ---- the grade-adjusted speed must agree with the VO2max builder's -----------------------------

TEST(RunStatsGap, MatchesWindowBuilderOnAMixedRun)
{
    RunStats      st;
    WindowBuilder wb;
    st.reset(kZones, 6);
    wb.reset();
    float d = 0.0f;
    for (unsigned i = 0; i < 1500; ++i) {
        Sample s = running(2.5f + 0.002f * static_cast<float>(i % 700), 150);
        s.gradeValid = (i % 5) != 0;
        s.gradePct   = static_cast<float>(static_cast<int>(i % 41) - 8);   // -8% to +32%: past the 20% cap
        if (i % 97 == 0) {
            s.speedValid = false;
        }
        if (i % 211 == 0) {
            s.deadReckoning = true;
        }
        if (i % 331 == 0) {
            s.speedMs = 0.2f;   // under the moving threshold
        }
        if (i % 400 == 399) {
            s.active = false;   // a pause
        }
        if (s.active) {
            d += s.speedMs;
        }
        st.addSecond(s, d);
        wb.addSecond(s);
    }
    EXPECT_NEAR(st.avgGradeAdjustedSpeedMs(), wb.avgGradeAdjustedSpeedMs(), 0.001);
    EXPECT_NEAR(st.result().gapSpeedMmps / 1000.0, wb.avgGradeAdjustedSpeedMs(), 0.001);
}

TEST(RunStatsGap, NothingMovingIsNone)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 30, 0.1f, 100);
    EXPECT_EQ(st.avgGradeAdjustedSpeedMs(), 0.0f);
    EXPECT_EQ(st.result().gapSpeedMmps, 0);
}

// ---- reset ----------------------------------------------------------------------------------

TEST(RunStats, AResetStartsANewRun)
{
    RunStats st;
    st.reset(kZones, 6);
    float d = 0.0f;
    feed(st, d, 1500, 4.0f, 150);
    ASSERT_GT(st.result().bestMs[0], 0u);
    const uint8_t other[3] = {100, 120, 140};
    st.reset(other, 3);
    const RunStatsResult r = st.result();
    EXPECT_EQ(r.activeSec, 0u);
    EXPECT_EQ(r.bestMs[0], 0u);
    EXPECT_EQ(zoneTotal(r), 0u);
    EXPECT_EQ(r.zoneCount, 3);
    float e = 0.0f;
    feed(st, e, 10, 3.0f, 125);   // above 120: its second zone
    EXPECT_EQ(st.result().zoneSec[2], 10u);
}

TEST(RunStats, TheResultIsSmallEnoughForAMessage)
{
    EXPECT_LE(sizeof(RunStatsResult), 120u);
}

// ---- the drift as text ----------------------------------------------------------------------

namespace
{
RunStatsResult withDrift(DriftStatus status, int16_t x10)
{
    RunStatsResult r;
    r.driftStatus = status;
    r.driftX10    = x10;
    return r;
}
}  // namespace

TEST(RunStatsText, DriftIsAPercentageToOneDecimal)
{
    char b[12];
    StatsText::drift(b, sizeof(b), withDrift(DriftStatus::Ok, 48));
    EXPECT_STREQ(b, "4.8%");
    StatsText::drift(b, sizeof(b), withDrift(DriftStatus::Ok, 0));
    EXPECT_STREQ(b, "0.0%");
    StatsText::drift(b, sizeof(b), withDrift(DriftStatus::Ok, -12));
    EXPECT_STREQ(b, "-1.2%");
    StatsText::drift(b, sizeof(b), withDrift(DriftStatus::Ok, -3));
    EXPECT_STREQ(b, "-0.3%");   // the sign survives a whole part of 0
    StatsText::drift(b, sizeof(b), withDrift(DriftStatus::Ok, 1000));
    EXPECT_STREQ(b, "100.0%");
}

TEST(RunStatsText, NoDriftReadsAsNone)
{
    char b[12];
    for (const DriftStatus st : {DriftStatus::TooShort, DriftStatus::NoHeartRate, DriftStatus::NotSteady,
                                 DriftStatus::Treadmill}) {
        StatsText::drift(b, sizeof(b), withDrift(st, 48));   // a stale number is never shown
        EXPECT_STREQ(b, "---");
    }
}

TEST(RunStatsText, TheReasonsFitTheirLine)
{
    EXPECT_STREQ(StatsText::driftReason(DriftStatus::Ok), "");
    for (const DriftStatus st : {DriftStatus::TooShort, DriftStatus::NoHeartRate, DriftStatus::NotSteady,
                                 DriftStatus::Treadmill}) {
        const char* why = StatsText::driftReason(st);
        EXPECT_GT(std::strlen(why), 0u);
        EXPECT_LE(std::strlen(why), 16u);   // one line of Regular16 inside the round face
    }
}

TEST(RunStatsText, TheTextIsBoundedByTheBuffer)
{
    char b[4];
    const size_t n = StatsText::drift(b, sizeof(b), withDrift(DriftStatus::Ok, 1000));
    EXPECT_EQ(n, std::strlen(b));
    EXPECT_LT(n, sizeof(b));
    EXPECT_EQ(StatsText::drift(nullptr, 4, withDrift(DriftStatus::Ok, 1)), 0u);
    char none[1];
    EXPECT_EQ(StatsText::drift(none, 0, withDrift(DriftStatus::Ok, 1)), 0u);
}

