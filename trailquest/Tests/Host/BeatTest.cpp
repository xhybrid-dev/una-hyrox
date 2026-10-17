// The timing beat: the needle, the zones, wrap-around of the sweep and of the
// kernel's millisecond clock, and the two-frame floor.

#include <gtest/gtest.h>

#include "Beat.hpp"

using namespace XTrailQuest;

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

TEST(Beat, ClampBoundsTheSweepAndTarget)
{
    BeatConfig c;
    c.sweepMs  = 10;
    c.targetMs = 5000;
    BeatConfig k = clampBeat(c);
    EXPECT_EQ(k.sweepMs, kMinSweepMs);
    EXPECT_LT(k.targetMs, k.sweepMs);

    c.sweepMs = 100000;
    k = clampBeat(c);
    EXPECT_EQ(k.sweepMs, kMaxSweepMs);
}

TEST(Beat, GoodNeverWiderThanTheSweep)
{
    BeatConfig c;
    c.sweepMs = 800;
    c.goodMs  = 5000;
    const BeatConfig k = clampBeat(c);
    EXPECT_LE(k.goodMs, k.sweepMs);
    EXPECT_LE(k.perfectMs, k.goodMs);
}

TEST(Beat, NeedleRunsAlongTheTrackAndStartsAgain)
{
    const BeatConfig c = defaults();   // 1600 ms
    EXPECT_EQ(beatPermille(c, 5000, 5000), 0u);
    EXPECT_EQ(beatPermille(c, 5000, 5800), 500u);
    EXPECT_EQ(beatPermille(c, 5000, 6599), 999u);
    EXPECT_EQ(beatPermille(c, 5000, 6600), 0u);       // the next pass
    EXPECT_EQ(beatPermille(c, 5000, 6600 + 800), 500u);
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

TEST(Beat, ATargetNearTheStartCanBeHitJustBeforeTheWrap)
{
    BeatConfig c;
    c.targetMs = 50;
    c = clampBeat(c);
    // 1550 ms into a 1600 ms sweep is 100 ms before the needle reaches 0 + 50.
    const BeatJudgement j = judgeBeat(c, 0, 1550);
    EXPECT_EQ(j.offsetMs, -100);
    EXPECT_EQ(j.result, BeatResult::Perfect);
    // And just after the wrap.
    EXPECT_EQ(judgeBeat(c, 0, 1600 + 140).result, BeatResult::Perfect);
    EXPECT_EQ(judgeBeat(c, 0, 1600 + 140).offsetMs, 90);
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

TEST(Beat, WorksAcrossTheWrapOfTheKernelClock)
{
    const BeatConfig c = defaults();
    const uint32_t start = 0xFFFFFF00u;   // 256 ms before the clock wraps
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
