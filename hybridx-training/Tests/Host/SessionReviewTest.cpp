#include "SessionReview.hpp"
#include "PaceModel.hpp"

#include <gtest/gtest.h>

using namespace XTraining;
using namespace XTraining::Review;

namespace {

void seconds(Accumulator& a, Cls c, int n, int32_t gap = 0, bool fastOnly = false)
{
    for (int i = 0; i < n; ++i) {
        a.addSecond(c, gap, fastOnly);
    }
}

}  // namespace

TEST(SessionReview, NothingJudgedIsNotAReview)
{
    Accumulator a;
    const Result r = a.finish();
    EXPECT_FALSE(r.judged);
    EXPECT_FALSE(r.offPace());
    EXPECT_EQ(r.dir(), Dir::None);
}

TEST(SessionReview, UnderThreeMinutesAndOneRepIsTooLittleToJudge)
{
    Accumulator a;
    seconds(a, Cls::In, 170);
    EXPECT_FALSE(a.finish().judged);
    seconds(a, Cls::In, 10);
    EXPECT_TRUE(a.finish().judged);
}

TEST(SessionReview, AllInBandIsOneHundredPercentWithNoDirection)
{
    Accumulator a;
    seconds(a, Cls::In, 600);
    const Result r = a.finish();
    ASSERT_TRUE(r.judged);
    EXPECT_EQ(r.inPct, 100);
    EXPECT_EQ(r.fastPct, 0);
    EXPECT_EQ(r.slowPct, 0);
    EXPECT_EQ(r.dir(), Dir::None);
    EXPECT_EQ(r.gapSecKm, 0);
    EXPECT_FALSE(r.offPace());
}

TEST(SessionReview, TooFastAllThroughIsOffPaceAndFast)
{
    Accumulator a;
    seconds(a, Cls::Fast, 300, -12);
    const Result r = a.finish();
    ASSERT_TRUE(r.judged);
    EXPECT_EQ(r.inPct, 0);
    EXPECT_EQ(r.fastPct, 100);
    EXPECT_EQ(r.dir(), Dir::Fast);
    EXPECT_EQ(r.gapSecKm, -12);
    EXPECT_TRUE(r.offPace());
}

TEST(SessionReview, TooSlowIsOffPaceAndSlowAndTheGapIsPositive)
{
    Accumulator a;
    seconds(a, Cls::Slow, 200, 8);
    seconds(a, Cls::In, 100);
    const Result r = a.finish();
    ASSERT_TRUE(r.judged);
    EXPECT_EQ(r.inPct, 33);
    EXPECT_EQ(r.slowPct, 67);
    EXPECT_EQ(r.dir(), Dir::Slow);
    EXPECT_EQ(r.gapSecKm, 8);
    EXPECT_TRUE(r.offPace());
}

TEST(SessionReview, TheSharesAlwaysAddToOneHundred)
{
    for (int in = 0; in <= 40; in += 3) {
        for (int fast = 0; fast <= 40; fast += 7) {
            for (int slow = 0; slow <= 40; slow += 5) {
                if (in + fast + slow < 200) {
                    continue;
                }
                Accumulator a;
                seconds(a, Cls::In, in);
                seconds(a, Cls::Fast, fast, -5);
                seconds(a, Cls::Slow, slow, 5);
                const Result r = a.finish();
                ASSERT_TRUE(r.judged);
                EXPECT_EQ(r.inPct + r.fastPct + r.slowPct, 100) << in << " " << fast << " " << slow;
            }
        }
    }
}

TEST(SessionReview, TheGapIsTheAverageMissInTheDominantDirection)
{
    Accumulator a;
    seconds(a, Cls::Slow, 100, 4);
    seconds(a, Cls::Slow, 100, 10);   // slow misses average 7
    seconds(a, Cls::Fast, 40, -30);   // a smaller number of fast ones do not change the direction
    seconds(a, Cls::In, 60);
    const Result r = a.finish();
    EXPECT_EQ(r.dir(), Dir::Slow);
    EXPECT_EQ(r.gapSecKm, 7);
}

TEST(SessionReview, AnEasyStepOnlyMissesWhenTooFast)
{
    Accumulator a;
    seconds(a, Cls::Slow, 200, 40, true);   // slower than the easy band: fine
    seconds(a, Cls::In, 100, 0, true);
    seconds(a, Cls::Fast, 100, -10, true);  // faster: a miss
    const Result r = a.finish();
    ASSERT_TRUE(r.judged);
    EXPECT_EQ(r.inPct, 75);
    EXPECT_EQ(r.fastPct, 25);
    EXPECT_EQ(r.slowPct, 0);
    EXPECT_EQ(r.dir(), Dir::Fast);
}

TEST(SessionReview, SecondsWithNoSignalAreNotCounted)
{
    Accumulator a;
    seconds(a, Cls::None, 1000);
    seconds(a, Cls::In, 200);
    const Result r = a.finish();
    EXPECT_EQ(r.judgedSec, 200);
    EXPECT_EQ(r.inPct, 100);
}

TEST(SessionReview, ShortRepsAreJudgedByTheirAverageAndWeightedByTheirTime)
{
    // 6 x 400 m, each about 96 s: five in band, one too fast.
    Accumulator a;
    for (int i = 0; i < 5; ++i) {
        a.addRep(Cls::In, 0, 96);
    }
    a.addRep(Cls::Fast, -9, 96);
    const Result r = a.finish();
    ASSERT_TRUE(r.judged);
    EXPECT_EQ(r.units, 6);
    EXPECT_EQ(r.unitsIn, 5);
    EXPECT_EQ(r.judgedSec, 576);
    EXPECT_EQ(r.inPct, 83);
    EXPECT_EQ(r.fastPct, 17);
    EXPECT_EQ(r.gapSecKm, -9);
    EXPECT_EQ(r.dir(), Dir::Fast);
    EXPECT_FALSE(r.offPace());
}

TEST(SessionReview, TwoRepsOfAMinuteCountAsJudgedWhereOneDoesNot)
{
    Accumulator a;
    a.addRep(Cls::In, 0, 40);
    EXPECT_FALSE(a.finish().judged);
    a.addRep(Cls::In, 0, 30);
    EXPECT_TRUE(a.finish().judged);   // two reps, 70 s
    Accumulator b;
    b.addRep(Cls::In, 0, 20);
    b.addRep(Cls::In, 0, 20);
    EXPECT_FALSE(b.finish().judged);   // 40 s is not enough even in two reps
}

TEST(SessionReview, ARepWithNoVerdictIsNotCounted)
{
    Accumulator a;
    a.addRep(Cls::None, 0, 100);
    a.addRep(Cls::In, 0, 0);
    EXPECT_EQ(a.finish().judgedSec, 0);
    EXPECT_EQ(a.finish().units, 0);
}

TEST(SessionReview, UnitsAreTheStepsSecondsClosedByEndUnit)
{
    // Four 3-minute reps judged by the clock: in, in, slow, slow.
    Accumulator a;
    for (int rep = 0; rep < 4; ++rep) {
        seconds(a, rep < 2 ? Cls::In : Cls::Slow, 180, rep < 2 ? 0 : 6);
        a.endUnit();
    }
    const Result r = a.finish();
    EXPECT_EQ(r.units, 4);
    EXPECT_EQ(r.unitsIn, 2);
    EXPECT_EQ(r.inPct, 50);
    // The open unit is counted without being closed.
    Accumulator b;
    seconds(b, Cls::In, 200);
    EXPECT_EQ(b.finish().units, 1);
    EXPECT_EQ(b.finish().units, 1);
}

TEST(SessionReview, ALaterRepSlowerThanTheFirstsIsAFade)
{
    // Six reps: the first two on pace, the last two 12 s/km slow.
    Accumulator a;
    const int32_t gaps[6] = {0, 0, 0, 0, 12, 12};
    for (int i = 0; i < 6; ++i) {
        a.addRep(gaps[i] == 0 ? Cls::In : Cls::Slow, gaps[i], 100);
    }
    const Result r = a.finish();
    EXPECT_EQ(r.fadeSecKm, 12);
    // The same reps the other way round: a negative fade (a negative split).
    Accumulator b;
    for (int i = 5; i >= 0; --i) {
        b.addRep(gaps[i] == 0 ? Cls::In : Cls::Slow, gaps[i], 100);
    }
    EXPECT_EQ(b.finish().fadeSecKm, -12);
}

TEST(SessionReview, UnderFourRepsThereIsNoFade)
{
    Accumulator a;
    a.addRep(Cls::In, 0, 100);
    a.addRep(Cls::Slow, 20, 100);
    a.addRep(Cls::Slow, 30, 100);
    EXPECT_EQ(a.finish().fadeSecKm, 0);
}

TEST(SessionReview, ThePaceOfARepJudgedByTimeBecomesSecondsPerKilometre)
{
    // 400 m at 3:55-4:10 /km is 94 to 100 s. A rep of 106 s is 6 s over the slow end: 15 s/km.
    SDK::Workout::Step s;
    s.end = SDK::Workout::StepEnd::Distance;
    s.target = SDK::Workout::Target::Speed;
    s.distanceCm = 40000;
    s.speedHighMmps = PaceModel::mmps(235);
    s.speedLowMmps  = PaceModel::mmps(250);
    const RepJudge::Result slow = RepJudge::judge(s, 106, 400, true);
    ASSERT_EQ(slow.verdict, RepJudge::Verdict::Slow);
    EXPECT_EQ(repGapSecKm(s, slow), 15);
    const RepJudge::Result fast = RepJudge::judge(s, 90, 400, true);
    ASSERT_EQ(fast.verdict, RepJudge::Verdict::Fast);
    EXPECT_EQ(repGapSecKm(s, fast), -10);   // 4 s under 94 s, over 400 m
    EXPECT_EQ(repGapSecKm(s, RepJudge::judge(s, 96, 400, true)), 0);
    EXPECT_EQ(repCls(slow.verdict), Cls::Slow);
    EXPECT_EQ(repCls(RepJudge::Verdict::None), Cls::None);
}

TEST(SessionReview, ThePaceOfARepJudgedByPaceIsAlreadySecondsPerKilometre)
{
    SDK::Workout::Step s;
    s.end = SDK::Workout::StepEnd::Time;
    s.target = SDK::Workout::Target::Speed;
    s.speedHighMmps = PaceModel::mmps(252);   // 4:12
    s.speedLowMmps  = PaceModel::mmps(264);   // 4:24
    const RepJudge::Result r = RepJudge::judge(s, 180, 700, false);   // 180 s / 0.7 km = 257 s/km: in band
    EXPECT_EQ(r.verdict, RepJudge::Verdict::InBand);
    EXPECT_EQ(repGapSecKm(s, r), 0);
    const RepJudge::Result f = RepJudge::judge(s, 180, 760, false);   // 237 s/km: 15 under the fast end
    ASSERT_EQ(f.verdict, RepJudge::Verdict::Fast);
    EXPECT_NEAR(repGapSecKm(s, f), -15, 1);
}

TEST(SessionReview, CountersDoNotOverflowOnAVeryLongSession)
{
    Accumulator a;
    for (int i = 0; i < 70000; ++i) {
        a.addSecond(Cls::In, 0);
    }
    const Result r = a.finish();
    EXPECT_EQ(r.judgedSec, 65535);   // capped, not wrapped
    EXPECT_EQ(r.inPct, 100);
}

TEST(SessionReview, MoreThanThirtyTwoUnitsStillCountAndNeverOverrun)
{
    Accumulator a;
    for (int i = 0; i < 40; ++i) {
        a.addRep(i < 20 ? Cls::In : Cls::Slow, i < 20 ? 0 : 10, 60);
    }
    const Result r = a.finish();
    EXPECT_EQ(r.units, 40);
    EXPECT_EQ(r.unitsIn, 20);
    EXPECT_EQ(r.judgedSec, 2400);
}

TEST(SessionReview, AbsurdGapsAreClamped)
{
    Accumulator a;
    seconds(a, Cls::Slow, 300, 100000);
    EXPECT_EQ(a.finish().gapSecKm, 600);
}

TEST(SessionReview, ResetStartsAgain)
{
    Accumulator a;
    seconds(a, Cls::Fast, 300, -10);
    a.reset();
    EXPECT_FALSE(a.finish().judged);
    EXPECT_EQ(a.finish().judgedSec, 0);
}
