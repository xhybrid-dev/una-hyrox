// The timing beat: the needle, the zones, the fold round the loop, the wrap of
// LVGL's tick, and the two-frame floor.

#include <gtest/gtest.h>

#include "Beat.hpp"

using namespace TrailQuest;

namespace
{
BeatConfig defaults() { return clampBeat(BeatConfig{}); }
}

TEST(Beat, DefaultsAreInRangeAndUnchangedByClamp)
{
    const BeatConfig c = BeatConfig{};
    const BeatConfig k = clampBeat(c);
    EXPECT_EQ(k.sweepMs, c.sweepMs);
    EXPECT_EQ(k.targetMs, c.targetMs);
    EXPECT_EQ(k.goodMs, c.goodMs);
    EXPECT_EQ(k.perfectMs, c.perfectMs);
}

TEST(Beat, ClampKeepsTheZonesAtLeastTwoFramesWide)
{
    BeatConfig c;
    c.perfectMs = 40;
    c.goodMs    = 60;
    const BeatConfig k = clampBeat(c);
    EXPECT_EQ(k.perfectMs, kMinPerfectMs);
    EXPECT_GE(k.goodMs, k.perfectMs);
}

TEST(Beat, ClampBoundsTheSweep)
{
    BeatConfig c;
    c.sweepMs = 10;
    EXPECT_EQ(clampBeat(c).sweepMs, kMinSweepMs);
    c.sweepMs = 100000;
    EXPECT_EQ(clampBeat(c).sweepMs, kMaxSweepMs);
}

TEST(Beat, GoodIsAtMostHalfTheSweepAndPerfectAtMostGood)
{
    BeatConfig c;
    c.sweepMs   = 800;
    c.goodMs    = 5000;
    c.perfectMs = 5000;
    const BeatConfig k = clampBeat(c);
    EXPECT_EQ(k.goodMs, 400u);
    EXPECT_EQ(k.perfectMs, 400u);
    EXPECT_GE(k.perfectMs, kMinPerfectMs);
}

TEST(Beat, ClampMovesTheTargetSoTheGoodZoneFitsOnTheTrack)
{
    BeatConfig c;   // good 400: the zone needs 200 ms either side
    c.targetMs = 50;
    EXPECT_EQ(clampBeat(c).targetMs, 200u);
    c.targetMs = 1590;
    EXPECT_EQ(clampBeat(c).targetMs, 1400u);
    c.targetMs = 100000;
    EXPECT_EQ(clampBeat(c).targetMs, 1400u);
}

TEST(Beat, EveryClampedConfigDrawsItsZonesInsideTheTrack)
{
    // A sweep of odd and even lengths, odd zone widths, targets everywhere.
    for (uint32_t sweep : {800u, 801u, 1600u, 1601u, 3999u, 4000u}) {
        for (uint32_t good : {0u, 201u, 400u, 999u, 5000u}) {
            for (uint32_t target : {0u, 1u, 199u, 400u, 1200u, 3999u, 0xFFFFFFFFu}) {
                BeatConfig c;
                c.sweepMs  = sweep;
                c.goodMs   = good;
                c.targetMs = target;
                const BeatConfig k = clampBeat(c);
                ASSERT_GE(k.targetMs, (k.goodMs + 1) / 2) << sweep << " " << good << " " << target;
                ASSERT_LE(k.targetMs + (k.goodMs + 1) / 2, k.sweepMs) << sweep << " " << good << " " << target;
                ASSERT_GE(k.targetMs, (k.perfectMs + 1) / 2);
            }
        }
    }
}

TEST(Beat, TrackPositionRunsFromStartToEnd)
{
    const BeatConfig c = defaults();   // 1600 ms
    EXPECT_EQ(beatToTrack(c, 0, 150), 0u);
    EXPECT_EQ(beatToTrack(c, 800, 150), 75u);
    EXPECT_EQ(beatToTrack(c, 1600, 150), 150u);
    EXPECT_EQ(beatToTrack(c, 9999, 150), 150u);     // held at the end
    EXPECT_EQ(beatToTrack(c, 1200, 1000), 750u);    // per mille
}

TEST(Beat, NeedleRunsAlongTheTrackAndStartsAgain)
{
    const BeatConfig c = defaults();   // 1600 ms
    EXPECT_EQ(beatPhaseMs(c, 5000, 5000), 0u);
    EXPECT_EQ(beatPhaseMs(c, 5000, 5800), 800u);
    EXPECT_EQ(beatPhaseMs(c, 5000, 6599), 1599u);
    EXPECT_EQ(beatPhaseMs(c, 5000, 6600), 0u);       // the next pass
    EXPECT_EQ(beatPhaseMs(c, 5000, 6600 + 800), 800u);
}

TEST(Beat, PressOnTheTargetIsPerfect)
{
    const BeatConfig c = defaults();
    const BeatJudgement j = judgeBeat(c, 1000, 1000 + c.targetMs);
    EXPECT_EQ(j.result, BeatResult::Perfect);
    EXPECT_EQ(j.offsetMs, 0);
}

TEST(Beat, ZoneEdgesAreInclusive)
{
    const BeatConfig c = defaults();   // perfect 200 (+-100), good 400 (+-200)
    EXPECT_EQ(judgeBeat(c, 0, c.targetMs + 100).result, BeatResult::Perfect);
    EXPECT_EQ(judgeBeat(c, 0, c.targetMs - 100).result, BeatResult::Perfect);
    EXPECT_EQ(judgeBeat(c, 0, c.targetMs + 101).result, BeatResult::Good);
    EXPECT_EQ(judgeBeat(c, 0, c.targetMs + 200).result, BeatResult::Good);
    EXPECT_EQ(judgeBeat(c, 0, c.targetMs - 200).result, BeatResult::Good);
    EXPECT_EQ(judgeBeat(c, 0, c.targetMs + 201).result, BeatResult::Miss);
    EXPECT_EQ(judgeBeat(c, 0, c.targetMs - 201).result, BeatResult::Miss);
}

TEST(Beat, OffsetIsSignedEarlyNegativeLatePositive)
{
    const BeatConfig c = defaults();
    EXPECT_EQ(judgeBeat(c, 0, c.targetMs - 150).offsetMs, -150);
    EXPECT_EQ(judgeBeat(c, 0, c.targetMs + 150).offsetMs, 150);
}

TEST(Beat, ThePressIsMeasuredTheNearWayRoundTheLoop)
{
    BeatConfig c;
    c.targetMs = 200;
    c = clampBeat(c);
    ASSERT_EQ(c.targetMs, 200u);
    // 1590 ms into a 1600 ms sweep is 210 ms before the next pass reaches 200.
    EXPECT_EQ(judgeBeat(c, 0, 1590).offsetMs, -210);
    EXPECT_EQ(judgeBeat(c, 0, 1590).result, BeatResult::Miss);
    // And 10 ms after the restart is 190 ms early: inside the good zone.
    EXPECT_EQ(judgeBeat(c, 0, 1610).offsetMs, -190);
    EXPECT_EQ(judgeBeat(c, 0, 1610).result, BeatResult::Good);
}

TEST(Beat, TheFoldIsExactForAnOddSweep)
{
    // The case the review found: a press exactly opposite the target. With a
    // 1601 ms sweep the near way is 800 ms early, not 801 ms late.
    BeatConfig c;
    c.sweepMs  = 1601;
    c.targetMs = 800;
    c.goodMs   = 800;
    c = clampBeat(c);
    ASSERT_EQ(c.sweepMs, 1601u);
    ASSERT_EQ(c.targetMs, 800u);
    EXPECT_EQ(judgeBeat(c, 0, 0).offsetMs, -800);
    EXPECT_EQ(judgeBeat(c, 0, 1600).offsetMs, 800);
    // Even sweep: exactly opposite folds to +sweep/2, as documented.
    const BeatConfig e = defaults();   // target 1200 of 1600
    EXPECT_EQ(judgeBeat(e, 0, 400).offsetMs, 800);
}

TEST(Beat, AnyPassOfTheNeedleCounts)
{
    const BeatConfig c = defaults();
    for (uint32_t pass = 0; pass < 5; ++pass) {
        EXPECT_EQ(judgeBeat(c, 777, 777 + pass * c.sweepMs + c.targetMs).result,
                  BeatResult::Perfect)
            << "pass " << pass;
    }
}

TEST(Beat, WorksAcrossTheWrapOfLvglsTick)
{
    // lv_tick_get() wraps at 2^32 (the SDK port's Port::tickCb).
    const BeatConfig c = defaults();
    const uint32_t start = 0xFFFFFF00u;   // 256 ms before the tick wraps
    const uint32_t press = start + c.targetMs;   // wraps past zero
    EXPECT_LT(press, start);
    const BeatJudgement j = judgeBeat(c, start, press);
    EXPECT_EQ(j.result, BeatResult::Perfect);
    EXPECT_EQ(j.offsetMs, 0);
    EXPECT_EQ(beatPhaseMs(c, start, press), c.targetMs);
}

TEST(Beat, PressedAtTheStartOfTheSweepIsAMissForTheDefaults)
{
    const BeatConfig c = defaults();   // target 1200 of 1600: 400 ms from the loop's start
    const BeatJudgement j = judgeBeat(c, 0, 0);
    EXPECT_EQ(j.result, BeatResult::Miss);
    EXPECT_EQ(j.offsetMs, 400);        // the near way round: 400 ms after the target, a pass on
}

TEST(Beat, StageOneIsTheOriginalBeat)
{
    const BeatConfig c = beatFor(0, 0);
    const BeatConfig d = defaults();
    EXPECT_EQ(c.sweepMs, d.sweepMs);
    EXPECT_EQ(c.goodMs, d.goodMs);
    EXPECT_EQ(c.perfectMs, d.perfectMs);
    EXPECT_EQ(c.targetMs, d.targetMs);
}

TEST(Beat, EachStageAndLapIsInRangeAndNoEasierThanTheLast)
{
    for (uint8_t lap = 0; lap < 4; ++lap) {
        for (uint8_t stage = 0; stage < 5; ++stage) {
            const BeatConfig c = beatFor(stage, lap);
            const BeatConfig k = clampBeat(c);
            EXPECT_EQ(c.sweepMs, k.sweepMs) << int(stage) << "/" << int(lap);
            EXPECT_EQ(c.goodMs, k.goodMs);
            EXPECT_EQ(c.targetMs, k.targetMs);
            EXPECT_EQ(c.perfectMs, kMinPerfectMs);   // two frames, the floor
            EXPECT_GE(c.goodMs, c.perfectMs);
            if (stage > 0) {   // each stage no easier than the one before it...
                EXPECT_LE(c.sweepMs, beatFor(stage - 1, lap).sweepMs);
                EXPECT_LE(c.goodMs, beatFor(stage - 1, lap).goodMs);
            }
            if (lap > 0) {     // ...and each lap no easier than the one before.
                EXPECT_LE(c.sweepMs, beatFor(stage, lap - 1).sweepMs);
                EXPECT_LE(c.goodMs, beatFor(stage, lap - 1).goodMs);
            }
        }
    }
}

TEST(Beat, TheLadderMatchesTheDesignAndHasAFloor)
{
    EXPECT_EQ(beatFor(1, 0).sweepMs, 1500u);
    EXPECT_EQ(beatFor(1, 0).goodMs, 360u);
    EXPECT_EQ(beatFor(4, 0).sweepMs, 1200u);
    EXPECT_EQ(beatFor(4, 0).goodMs, 240u);
    EXPECT_EQ(beatFor(4, 1).sweepMs, 1100u);
    // Far out: the floors hold, and a stage or lap past the end is the last.
    EXPECT_EQ(beatFor(4, 20).sweepMs, 1000u);
    EXPECT_EQ(beatFor(4, 20).goodMs, 200u);
    EXPECT_EQ(beatFor(200, 255).sweepMs, 1000u);
}
