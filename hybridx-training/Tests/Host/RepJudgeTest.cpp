#include "RepJudge.hpp"
#include "PaceModel.hpp"

#include <gtest/gtest.h>

using namespace XTraining;
using SDK::Workout::Step;
using SDK::Workout::StepEnd;
using SDK::Workout::Target;

namespace {
// A rep with a pace band: @p fast and @p slow in s/km.
Step rep(StepEnd end, uint16_t fast, uint16_t slow)
{
    Step s;
    s.end = end;
    s.target = Target::Speed;
    s.speedHighMmps = PaceModel::mmps(fast);
    s.speedLowMmps  = PaceModel::mmps(slow);
    return s;
}
}  // namespace

TEST(RepJudge, ADistanceRepThatRanItsDistanceIsJudgedByItsTime)
{
    // 400 m at 3:55-4:10 /km: 1:34 to 1:40.
    Step s = rep(StepEnd::Distance, 235, 250);
    s.distanceCm = 40000;
    RepJudge::Result r = RepJudge::judge(s, 96, 400, true);
    ASSERT_TRUE(r.ok);
    EXPECT_TRUE(r.byTime);
    EXPECT_EQ(r.value, 96u);
    EXPECT_EQ(r.fastEnd, 94u);
    EXPECT_EQ(r.slowEnd, 100u);
    EXPECT_EQ(r.verdict, RepJudge::Verdict::InBand);
    EXPECT_EQ(RepJudge::judge(s, 92, 400, true).verdict, RepJudge::Verdict::Fast);
    EXPECT_EQ(RepJudge::judge(s, 103, 400, true).verdict, RepJudge::Verdict::Slow);
}

TEST(RepJudge, ATimedRepOrOneEndedByHandIsJudgedByItsPace)
{
    // 30 s at 4:12-4:24 /km, 120 m: 4:10 /km, a little fast.
    Step s = rep(StepEnd::Time, 252, 264);
    RepJudge::Result r = RepJudge::judge(s, 30, 120, true);
    ASSERT_TRUE(r.ok);
    EXPECT_FALSE(r.byTime);
    EXPECT_EQ(r.value, 250u);
    EXPECT_EQ(r.fastEnd, 252u);
    EXPECT_EQ(r.slowEnd, 264u);
    EXPECT_EQ(r.verdict, RepJudge::Verdict::Fast);
    // A 400 m rep ended by the lap button at 300 m: by pace.
    Step d = rep(StepEnd::Distance, 235, 250);
    d.distanceCm = 40000;
    r = RepJudge::judge(d, 72, 300, false);
    EXPECT_FALSE(r.byTime);
    EXPECT_EQ(r.value, 240u);
    EXPECT_EQ(r.verdict, RepJudge::Verdict::InBand);
}

TEST(RepJudge, NoBandNoVerdictAndTooLittleNothing)
{
    Step stride;
    stride.end = StepEnd::Time;
    stride.target = Target::Open;
    RepJudge::Result r = RepJudge::judge(stride, 20, 110, true);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.value, 182u);   // 3:02 /km
    EXPECT_EQ(r.fastEnd, 0u);
    EXPECT_EQ(r.verdict, RepJudge::Verdict::None);
    EXPECT_FALSE(RepJudge::judge(stride, 20, 10, true).ok);   // no GPS
    EXPECT_FALSE(RepJudge::judge(stride, 0, 100, true).ok);
}
