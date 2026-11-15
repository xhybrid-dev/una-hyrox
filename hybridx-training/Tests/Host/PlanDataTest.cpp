// The generated plans keep the rules the generator checks, and the ones that
// need the whole plan in view.

#include "data/PlanData.hpp"

#include <gtest/gtest.h>

using namespace XTraining;

namespace {

uint32_t quantity(const Session& s)
{
    // A rough size of a week's long run, to compare weeks with.
    return s.kind == Kind::Long || s.kind == Kind::LongFinish ? s.a : 0;
}

}  // namespace

TEST(PlanData, ThreePlansWithTheirIds)
{
    ASSERT_EQ(PlanData::kPlanCount, 3);
    ASSERT_NE(PlanData::find(PlanId::TenK), nullptr);
    ASSERT_NE(PlanData::find(PlanId::Half), nullptr);
    ASSERT_NE(PlanData::find(PlanId::Vo2max), nullptr);
    EXPECT_EQ(PlanData::find(PlanId::None), nullptr);
    EXPECT_EQ(PlanData::find(PlanId::TenK)->weekCount, 10);
    EXPECT_EQ(PlanData::find(PlanId::Half)->weekCount, 12);
    EXPECT_EQ(PlanData::find(PlanId::Vo2max)->weekCount, 8);
    EXPECT_EQ(PlanData::find(PlanId::Half)->raceCm, 2109750u);
}

TEST(PlanData, EveryWeekKeepsTheRules)
{
    for (const PlanDef& p : PlanData::kPlans) {
        SCOPED_TRACE(p.name);
        for (uint8_t w = 0; w < p.weekCount; ++w) {
            SCOPED_TRACE(w + 1);
            const Week& wk = p.weeks[w];
            ASSERT_LT(wk.key, kSlotCount);
            EXPECT_TRUE(isHard(wk.slot[wk.key].kind));
            EXPECT_FALSE(isHard(wk.slot[kSlotEasy].kind));
            const Kind lk = wk.slot[kSlotLong].kind;
            EXPECT_TRUE(lk == Kind::Long || lk == Kind::LongFinish || lk == Kind::TimeTrial5k
                        || lk == Kind::TimeTrial10k || lk == Kind::Race);
            if (lk == Kind::LongFinish) {
                const int hard = isHard(wk.slot[kSlotQ1].kind) + isHard(wk.slot[kSlotQ2].kind);
                EXPECT_LE(hard, 1) << "C8";
            }
        }
    }
}

TEST(PlanData, CutBackWeeksAreLighterThanTheWeekBefore)
{
    for (const PlanDef& p : PlanData::kPlans) {
        SCOPED_TRACE(p.name);
        for (uint8_t w = 1; w < p.weekCount; ++w) {
            if (p.weeks[w].phase != Phase::CutBack) {
                continue;
            }
            const Week& cut = p.weeks[w];
            const Week& before = p.weeks[w - 1];
            // Its long run (if it has one) is shorter, and its quality no more.
            EXPECT_LE(quantity(cut.slot[kSlotLong]), quantity(before.slot[kSlotLong])) << w + 1;
            const int hardCut = isHard(cut.slot[kSlotQ1].kind) + isHard(cut.slot[kSlotQ2].kind);
            const int hardBefore = isHard(before.slot[kSlotQ1].kind) + isHard(before.slot[kSlotQ2].kind);
            EXPECT_LE(hardCut, hardBefore) << w + 1;
        }
    }
}

TEST(PlanData, RacePlansEndWithTheRace)
{
    for (const PlanDef& p : PlanData::kPlans) {
        const Kind last = p.weeks[p.weekCount - 1].slot[kSlotLong].kind;
        if (p.raceCm > 0) {
            EXPECT_EQ(last, Kind::Race) << p.name;
            EXPECT_NE(p.trialWeek, nullptr) << p.name;
        } else {
            EXPECT_EQ(last, Kind::TimeTrial5k) << p.name;  // the block's retest
            EXPECT_EQ(p.weeks[0].slot[kSlotLong].kind, Kind::TimeTrial5k) << p.name;
        }
        EXPECT_GE(p.minWeeks, 1);
        EXPECT_LE(p.minWeeks, p.weekCount);
    }
}

TEST(PlanData, LongRunsStayInsideTheirLevelRange)
{
    for (const PlanDef& p : PlanData::kPlans) {
        const LongRange& r = p.longRange[1];
        uint16_t peak = 0;
        for (uint8_t w = 0; w < p.weekCount; ++w) {
            const Session& s = p.weeks[w].slot[kSlotLong];
            if (s.kind == Kind::Long || s.kind == Kind::LongFinish) {
                peak = s.a > peak ? s.a : peak;
            }
        }
        EXPECT_EQ(peak, r.peak) << p.name << ": the plan file's peak long run is level 2's peak";
    }
}
