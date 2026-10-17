/**
 * @file PacePlanTest.cpp
 * @brief The pace plan: which targets each mode builds, and the delta and projection that follow
 *        from the race model's own state.
 *
 * Nothing here knows HYROX pacing. Every number is made up for the test, or taken from a "last
 * race" the test builds.
 */

#include <gtest/gtest.h>

#include "PacePlan.hpp"
#include "RaceModel.hpp"

#include <vector>

using Race::Format;
using Race::PaceMode;
using Race::PacePlan;
using Race::RaceModel;
using Race::SegmentDesc;
using Race::SegmentType;
using Status = Race::PacePlan::Status;

namespace
{

constexpr uint32_t kSec = 1000u;

std::vector<SegmentDesc> templateFor(Format f, bool roxzone)
{
    SegmentDesc buf[Race::kMaxSegments];
    const uint8_t n = RaceModel::buildTemplate(f, roxzone, buf, Race::kMaxSegments);
    return std::vector<SegmentDesc>(buf, buf + n);
}

/// A saved race over @p tmpl, each segment taking @p secs(i) seconds.
template <typename F>
ActivitySummary summaryOver(const std::vector<SegmentDesc> &tmpl, F secs, uint16_t runM = 1000u)
{
    ActivitySummary s;
    s.valid = true;
    s.completed = true;
    s.runDistanceM = runM;
    s.count = static_cast<uint8_t>(tmpl.size());
    for (size_t i = 0; i < tmpl.size(); ++i) {
        s.segments[i].type = static_cast<uint8_t>(tmpl[i].type);
        s.segments[i].round = tmpl[i].round;
        s.segments[i].stationId = tmpl[i].stationId;
        s.segments[i].durationMs = secs(i) * kSec;
        s.totalMs += s.segments[i].durationMs;
    }
    return s;
}

uint32_t flat100(size_t) { return 100u; }

/// A saved Full race with distinct, easy-to-check times: segment i takes 100 + i seconds.
ActivitySummary lastFull(bool roxzone = false, uint16_t runM = 1000u)
{
    return summaryOver(templateFor(Format::Full, roxzone),
                       [](size_t i) { return static_cast<uint32_t>(100u + i); }, runM);
}

RaceModel::Config config(Format f = Format::Full, bool rox = false, uint32_t lockoutMs = 3000u)
{
    RaceModel::Config c;
    c.format = f;
    c.roxzone = rox;
    c.lockoutMs = lockoutMs;
    return c;
}

PacePlan planFrom(PaceMode mode, Format f, bool rox, const ActivitySummary &last,
                  uint32_t targetMs = 0u, uint16_t runM = 1000u)
{
    PacePlan p;
    const std::vector<SegmentDesc> t = templateFor(f, rox);
    p.build(mode, t.data(), static_cast<uint8_t>(t.size()), last, targetMs, runM);
    return p;
}

}  // namespace

// -- Availability ---------------------------------------------------------------------------

TEST(PacePlan, OffIsInactive)
{
    PacePlan p = planFrom(PaceMode::Off, Format::Full, false, lastFull());
    EXPECT_FALSE(p.active());
    EXPECT_EQ(p.status(), Status::Off);
    EXPECT_EQ(p.targetMs(0), 0u);
}

TEST(PacePlan, NoSavedRaceMeansNoPlan)
{
    ActivitySummary none;   // valid = false
    for (PaceMode m : {PaceMode::LastRace, PaceMode::Target}) {
        PacePlan p = planFrom(m, Format::Full, false, none, 90u * 60u * kSec);
        EXPECT_FALSE(p.active());
        EXPECT_EQ(p.status(), Status::NoLastRace);
    }
    ActivitySummary empty;
    empty.valid = true;     // valid, but no segments
    EXPECT_EQ(planFrom(PaceMode::LastRace, Format::Full, false, empty).status(),
              Status::NoLastRace);
}

TEST(PacePlan, BuildClearsAnEarlierPlan)
{
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false, lastFull());
    ASSERT_TRUE(p.active());
    const std::vector<SegmentDesc> t = templateFor(Format::Full, false);
    p.build(PaceMode::Off, t.data(), static_cast<uint8_t>(t.size()), lastFull(), 0u, 1000u);
    EXPECT_FALSE(p.active());
    EXPECT_EQ(p.targetMs(0), 0u);
}

// -- Last race ------------------------------------------------------------------------------

TEST(PacePlan, LastRaceTargetsAreTheLastRacesTimes)
{
    const ActivitySummary last = lastFull();
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false, last);
    ASSERT_TRUE(p.active());
    ASSERT_EQ(p.count(), 16);
    for (uint8_t i = 0; i < 16; ++i) {
        EXPECT_EQ(p.targetMs(i), last.segments[i].durationMs) << int(i);
    }
    EXPECT_EQ(p.targetMs(16), 0u);   // out of range
}

TEST(PacePlan, AHalfCanUseAFullAsItsPlan)
{
    const ActivitySummary last = lastFull();
    PacePlan b = planFrom(PaceMode::LastRace, Format::HalfB, false, last);
    ASSERT_TRUE(b.active());
    ASSERT_EQ(b.count(), 8);
    // Half B is rounds 5 to 8: the Full's segments 8..15.
    for (uint8_t i = 0; i < 8; ++i) {
        EXPECT_EQ(b.targetMs(i), last.segments[8 + i].durationMs) << int(i);
    }
}

TEST(PacePlan, ASegmentTheLastRaceLackedHasNoTarget)
{
    // Last race was a Half A; this one is a Full. Rounds 5-8 have nothing to match.
    const ActivitySummary last = summaryOver(templateFor(Format::HalfA, false), flat100);
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false, last);
    ASSERT_TRUE(p.active());
    for (uint8_t i = 0; i < 8; ++i) {
        EXPECT_EQ(p.targetMs(i), 100u * kSec);
    }
    for (uint8_t i = 8; i < 16; ++i) {
        EXPECT_EQ(p.targetMs(i), 0u) << int(i);
    }
}

TEST(PacePlan, NothingMatchingMeansNoPlan)
{
    // The last race was Half A; this one is Half B: no round in common.
    const ActivitySummary last = summaryOver(templateFor(Format::HalfA, false), flat100);
    PacePlan p = planFrom(PaceMode::LastRace, Format::HalfB, false, last);
    EXPECT_FALSE(p.active());
    EXPECT_EQ(p.status(), Status::NoMatch);
}

TEST(PacePlan, RoxzoneSegmentsOnlyGetTargetsIfTheLastRaceHadThem)
{
    // Last race without Roxzone, this race with: runs and stations match, Roxzone does not.
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, true, lastFull(false));
    ASSERT_TRUE(p.active());
    const std::vector<SegmentDesc> t = templateFor(Format::Full, true);
    for (uint8_t i = 0; i < t.size(); ++i) {
        const bool rox = t[i].type == SegmentType::RoxIn || t[i].type == SegmentType::RoxOut;
        EXPECT_EQ(p.targetMs(i) == 0u, rox) << int(i);
    }
}

TEST(PacePlan, ARunIsScaledByRunLengthAndNothingElseIs)
{
    // Last race: 1 km runs. This race: 500 m runs.
    const ActivitySummary last = lastFull(false, 1000u);
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false, last, 0u, 500u);
    ASSERT_TRUE(p.active());
    EXPECT_EQ(p.targetMs(0), last.segments[0].durationMs / 2u);    // a run: halved
    EXPECT_EQ(p.targetMs(1), last.segments[1].durationMs);         // a station: unchanged
    EXPECT_EQ(p.targetMs(2), last.segments[2].durationMs / 2u);

    // And the other way: last race 500 m, this one 1 km.
    const ActivitySummary shortLast = lastFull(false, 500u);
    PacePlan q = planFrom(PaceMode::LastRace, Format::Full, false, shortLast, 0u, 1000u);
    EXPECT_EQ(q.targetMs(0), shortLast.segments[0].durationMs * 2u);
    EXPECT_EQ(q.targetMs(1), shortLast.segments[1].durationMs);
}

TEST(PacePlan, ASummaryWithoutARunLengthIsReadAsOneKilometre)
{
    ActivitySummary old = lastFull();
    old.runDistanceM = 0u;   // saved before the plan existed
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false, old, 0u, 1000u);
    EXPECT_EQ(p.targetMs(0), old.segments[0].durationMs);
}

// -- Target finish --------------------------------------------------------------------------

TEST(PacePlan, TargetTargetsAddUpToTheTargetTime)
{
    const uint32_t target = 95u * 60u * kSec;
    PacePlan p = planFrom(PaceMode::Target, Format::Full, false, lastFull(), target);
    ASSERT_TRUE(p.active());
    uint64_t sum = 0;
    for (uint8_t i = 0; i < p.count(); ++i) {
        EXPECT_GT(p.targetMs(i), 0u);
        sum += p.targetMs(i);
    }
    EXPECT_EQ(sum, target);
}

TEST(PacePlan, TargetKeepsTheShapeOfTheLastRace)
{
    // Twice the last race's total: every target is twice the last race's time (to a millisecond).
    const ActivitySummary last = lastFull();
    PacePlan p = planFrom(PaceMode::Target, Format::Full, false, last, last.totalMs * 2u);
    ASSERT_TRUE(p.active());
    for (uint8_t i = 0; i + 1 < p.count(); ++i) {
        EXPECT_NEAR(static_cast<double>(p.targetMs(i)),
                    2.0 * last.segments[i].durationMs, 1.0) << int(i);
    }
}

TEST(PacePlan, TargetNeedsATargetTime)
{
    PacePlan p = planFrom(PaceMode::Target, Format::Full, false, lastFull(), 0u);
    EXPECT_FALSE(p.active());
    EXPECT_EQ(p.status(), Status::NoTarget);
}

TEST(PacePlan, TargetNeedsTheLastRaceToCoverTheWholeRace)
{
    const uint32_t target = 60u * 60u * kSec;
    const ActivitySummary half = summaryOver(templateFor(Format::HalfA, false), flat100);

    // A Full from a Half: rounds 5-8 are missing, the shape cannot add up to the target.
    PacePlan full = planFrom(PaceMode::Target, Format::Full, false, half, target);
    EXPECT_FALSE(full.active());
    EXPECT_EQ(full.status(), Status::NoMatch);

    // Roxzone on, last race without it: same.
    PacePlan rox = planFrom(PaceMode::Target, Format::Full, true, lastFull(false), target);
    EXPECT_FALSE(rox.active());
    EXPECT_EQ(rox.status(), Status::NoMatch);

    // The other way round is covered: a Half from a Full, Roxzone off from Roxzone on.
    EXPECT_TRUE(planFrom(PaceMode::Target, Format::HalfB, false, lastFull(), target).active());
    EXPECT_TRUE(planFrom(PaceMode::Target, Format::Full, false, lastFull(true), target).active());
}

TEST(PacePlan, TargetScalesRunsForRunLength)
{
    // 500 m runs against a last race with 1 km runs: the last race's runs count half.
    const ActivitySummary last = lastFull(false, 1000u);
    PacePlan p = planFrom(PaceMode::Target, Format::Full, false, last, 3600u * kSec, 500u);
    ASSERT_TRUE(p.active());
    // A run's share is half what it was, a station's unchanged, so a station now has more than
    // twice a run's share relative to the last race.
    const double runRatio = static_cast<double>(p.targetMs(0)) / last.segments[0].durationMs;
    const double stationRatio = static_cast<double>(p.targetMs(1)) / last.segments[1].durationMs;
    EXPECT_NEAR(runRatio / stationRatio, 0.5, 0.01);
}

// -- Live: delta and projection -------------------------------------------------------------

TEST(PacePlanLive, NothingBeforeThePlanIsActive)
{
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    PacePlan p;
    const PacePlan::Live l = p.live(m, 10u * kSec);
    EXPECT_FALSE(l.hasDelta);
    EXPECT_FALSE(l.hasProjection);
}

TEST(PacePlanLive, DeltaIsActualMinusTargetAtASplit)
{
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false,
                          summaryOver(templateFor(Format::Full, false), flat100));
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    ASSERT_TRUE(m.split(90u * kSec));    // 10 s ahead
    PacePlan::Live l = p.live(m, 90u * kSec);
    ASSERT_TRUE(l.hasDelta);
    EXPECT_EQ(l.deltaMs, -10 * 1000);

    ASSERT_TRUE(m.split(90u * kSec + 120u * kSec));   // the next took 120 s: 20 s over
    l = p.live(m, 210u * kSec);
    EXPECT_EQ(l.deltaMs, +10 * 1000);
}

TEST(PacePlanLive, WithinASegmentOnlyTimeOverTheTargetCounts)
{
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false,
                          summaryOver(templateFor(Format::Full, false), flat100));
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    ASSERT_TRUE(m.split(90u * kSec));    // -10 s banked

    // 50 s into the next segment (target 100 s): ahead is not credited halfway through.
    EXPECT_EQ(p.live(m, 140u * kSec).deltaMs, -10 * 1000);
    // 130 s into it: 30 s over its target on top of the banked -10 s.
    EXPECT_EQ(p.live(m, 220u * kSec).deltaMs, +20 * 1000);
}

TEST(PacePlanLive, ProjectionHoldsThePlanFromHere)
{
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false,
                          summaryOver(templateFor(Format::Full, false), flat100));
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    // 40 s into segment 0: 40 s done, 60 s left of it, 15 more at 100 s: 1600 s.
    PacePlan::Live l = p.live(m, 40u * kSec);
    ASSERT_TRUE(l.hasProjection);
    EXPECT_EQ(l.projectedMs, 1600u * kSec);
    EXPECT_EQ(l.segmentTargetMs, 100u * kSec);

    // Past its target: nothing left of it, so 130 + 15 x 100.
    l = p.live(m, 130u * kSec);
    EXPECT_EQ(l.projectedMs, 1630u * kSec);
}

TEST(PacePlanLive, ProjectionIsWithheldWhenARemainingSegmentHasNoTarget)
{
    // A Full with a Half A's targets: rounds 5-8 have none.
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false,
                          summaryOver(templateFor(Format::HalfA, false), flat100));
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));

    PacePlan::Live l = p.live(m, 40u * kSec);
    EXPECT_EQ(l.segmentTargetMs, 100u * kSec);   // segment 0 has a target...
    EXPECT_TRUE(l.hasDelta);                    // ...so a delta can be shown,
    EXPECT_FALSE(l.hasProjection);                // but not a projection
}

TEST(PacePlanLive, UndoRestoresTheDelta)
{
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false,
                          summaryOver(templateFor(Format::Full, false), flat100));
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    const PacePlan::Live before = p.live(m, 60u * kSec);

    ASSERT_TRUE(m.split(60u * kSec));
    ASSERT_TRUE(m.undoSplit());
    const PacePlan::Live after = p.live(m, 60u * kSec);

    EXPECT_EQ(after.deltaMs, before.deltaMs);
    EXPECT_EQ(after.projectedMs, before.projectedMs);
    EXPECT_EQ(after.segmentTargetMs, before.segmentTargetMs);
}

TEST(PacePlanLive, PausedTimeIsNotBehind)
{
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false,
                          summaryOver(templateFor(Format::Full, false), flat100));
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    ASSERT_TRUE(m.pause(50u * kSec));
    ASSERT_TRUE(m.resume(200u * kSec));     // 150 s paused
    ASSERT_TRUE(m.split(250u * kSec));      // 50 + 50 s active = 100 s: exactly on target

    const PacePlan::Live l = p.live(m, 250u * kSec);
    ASSERT_TRUE(l.hasDelta);
    EXPECT_EQ(l.deltaMs, 0);
}

TEST(PacePlanLive, ASplitTheLockoutIgnoredChangesNothing)
{
    PacePlan p = planFrom(PaceMode::LastRace, Format::Full, false,
                          summaryOver(templateFor(Format::Full, false), flat100));
    RaceModel m;
    ASSERT_TRUE(m.start(config(), 0u));
    ASSERT_TRUE(m.split(90u * kSec));
    const PacePlan::Live before = p.live(m, 91u * kSec);

    EXPECT_FALSE(m.split(91u * kSec));      // inside the 3 s lockout
    const PacePlan::Live after = p.live(m, 91u * kSec);
    EXPECT_EQ(after.deltaMs, before.deltaMs);
    EXPECT_EQ(m.recordedCount(), 1);
}

TEST(PacePlanLive, AFinishedRaceKeepsItsDeltaAndHasNoProjection)
{
    // A Half A of eight 100 s segments, run at 90 s each: 80 s ahead at the finish.
    PacePlan p = planFrom(PaceMode::LastRace, Format::HalfA, false,
                          summaryOver(templateFor(Format::HalfA, false), flat100));
    RaceModel m;
    ASSERT_TRUE(m.start(config(Format::HalfA), 0u));
    uint32_t t = 0u;
    for (int i = 0; i < 8; ++i) {
        t += 90u * kSec;
        ASSERT_TRUE(m.split(t)) << i;
    }
    ASSERT_EQ(m.state(), RaceModel::State::Finished);

    const PacePlan::Live l = p.live(m, t);
    ASSERT_TRUE(l.hasDelta);
    EXPECT_EQ(l.deltaMs, -80 * 1000);
    EXPECT_FALSE(l.hasProjection);
}
