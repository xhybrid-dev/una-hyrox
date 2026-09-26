#include <gtest/gtest.h>

#include "TargetEvaluator.hpp"
#include "WorkoutTypes.hpp"

using namespace Intervals;

TEST(Classify, OpenTargetIsAlwaysNoTarget)
{
    Target target { TargetKind::Open, 0, 0 };
    Sample sample { true, 300, true, 150, 3 };
    EXPECT_EQ(classify(target, sample), ZoneState::NoTarget);
}

TEST(Classify, PaceWithoutASampleIsNoSample)
{
    Target target { TargetKind::Pace, 250, 320 };
    Sample sample;   // hasPace = false
    EXPECT_EQ(classify(target, sample), ZoneState::NoSample);
}

TEST(Classify, PaceSlowerThanTheSlowBoundIsUnder)
{
    // low=250 (fast), high=320 (slow) sec/km.
    Target target { TargetKind::Pace, 250, 320 };
    Sample sample;
    sample.hasPace      = true;
    sample.paceSecPerKm = 340;   // slower than 320: under effort
    EXPECT_EQ(classify(target, sample), ZoneState::Under);
}

TEST(Classify, PaceFasterThanTheFastBoundIsOver)
{
    Target target { TargetKind::Pace, 250, 320 };
    Sample sample;
    sample.hasPace      = true;
    sample.paceSecPerKm = 200;   // faster than 250: over effort
    EXPECT_EQ(classify(target, sample), ZoneState::Over);
}

TEST(Classify, PaceInsideTheBandIsInZone)
{
    Target target { TargetKind::Pace, 250, 320 };
    Sample sample;
    sample.hasPace      = true;
    sample.paceSecPerKm = 290;
    EXPECT_EQ(classify(target, sample), ZoneState::InZone);
}

TEST(Classify, HeartRateZoneWithoutASampleIsNoSample)
{
    Target target { TargetKind::HeartRateZone, 2, 3 };
    Sample sample;   // hasHr = false
    EXPECT_EQ(classify(target, sample), ZoneState::NoSample);
}

TEST(Classify, HeartRateZoneBelowIsUnderAboveIsOver)
{
    Target target { TargetKind::HeartRateZone, 2, 3 };
    Sample under;
    under.hasHr  = true;
    under.hrZone = 1;
    EXPECT_EQ(classify(target, under), ZoneState::Under);

    Sample over;
    over.hasHr  = true;
    over.hrZone = 4;
    EXPECT_EQ(classify(target, over), ZoneState::Over);

    Sample inZone;
    inZone.hasHr  = true;
    inZone.hrZone = 2;
    EXPECT_EQ(classify(target, inZone), ZoneState::InZone);
}

TEST(Classify, HeartRateBpmBelowIsUnderAboveIsOver)
{
    Target target { TargetKind::HeartRateBpm, 140, 160 };
    Sample under;
    under.hasHr = true;
    under.hrBpm = 130;
    EXPECT_EQ(classify(target, under), ZoneState::Under);

    Sample over;
    over.hasHr = true;
    over.hrBpm = 170;
    EXPECT_EQ(classify(target, over), ZoneState::Over);

    Sample inZone;
    inZone.hasHr = true;
    inZone.hrBpm = 150;
    EXPECT_EQ(classify(target, inZone), ZoneState::InZone);
}

TEST(CueDebouncer, DoesNotFireBeforeThreeConsecutiveAgreeingTicks)
{
    CueDebouncer debouncer;
    ZoneState    out;
    EXPECT_FALSE(debouncer.update(ZoneState::Under, out));
    EXPECT_FALSE(debouncer.update(ZoneState::Under, out));
    // Third consecutive agreeing tick fires.
    EXPECT_TRUE(debouncer.update(ZoneState::Under, out));
    EXPECT_EQ(out, ZoneState::Under);
    EXPECT_EQ(debouncer.current(), ZoneState::Under);
}

TEST(CueDebouncer, ADifferingIntermediateSampleResetsTheRun)
{
    CueDebouncer debouncer;
    ZoneState    out;
    EXPECT_FALSE(debouncer.update(ZoneState::Under, out));
    EXPECT_FALSE(debouncer.update(ZoneState::Under, out));
    EXPECT_FALSE(debouncer.update(ZoneState::InZone, out));   // resets the run
    EXPECT_FALSE(debouncer.update(ZoneState::Under, out));    // back to 1
    EXPECT_FALSE(debouncer.update(ZoneState::Under, out));    // 2
    EXPECT_TRUE(debouncer.update(ZoneState::Under, out));     // 3: fires
    EXPECT_EQ(out, ZoneState::Under);
}

TEST(CueDebouncer, AlreadyCommittedStateNeverFiresAgain)
{
    CueDebouncer debouncer;
    ZoneState    out;
    debouncer.update(ZoneState::Over, out);
    debouncer.update(ZoneState::Over, out);
    ASSERT_TRUE(debouncer.update(ZoneState::Over, out));
    // Same state again and again: no further fire.
    EXPECT_FALSE(debouncer.update(ZoneState::Over, out));
    EXPECT_FALSE(debouncer.update(ZoneState::Over, out));
    EXPECT_FALSE(debouncer.update(ZoneState::Over, out));
}

TEST(CueDebouncer, TransitionsToASecondStateAfterCommitting)
{
    CueDebouncer debouncer;
    ZoneState    out;
    debouncer.update(ZoneState::Under, out);
    debouncer.update(ZoneState::Under, out);
    ASSERT_TRUE(debouncer.update(ZoneState::Under, out));

    EXPECT_FALSE(debouncer.update(ZoneState::InZone, out));
    EXPECT_FALSE(debouncer.update(ZoneState::InZone, out));
    ASSERT_TRUE(debouncer.update(ZoneState::InZone, out));
    EXPECT_EQ(out, ZoneState::InZone);
}
