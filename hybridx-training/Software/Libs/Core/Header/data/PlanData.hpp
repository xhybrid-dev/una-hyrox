/**
 * @file    PlanData.hpp
 * @brief   The plans, generated from the .plan files in Tools/Plans by gen_plans.py.
 *
 * DO NOT EDIT: change the plan files and run gen_plans.py. The host test
 * plan-data-current fails if this file and the plan files disagree.
 */

#ifndef XTRAINING_PLAN_DATA_HPP
#define XTRAINING_PLAN_DATA_HPP

#include "../PlanTypes.hpp"

namespace XTraining {
namespace PlanData {

// 10k.plan
inline constexpr Week k10kWeeks[] = {
    Week{Phase::Base, 2, {Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::Easy, 0, 35, 0, 0}, Session{Kind::Cruise, 3, 360, 0, 0}, Session{Kind::Long, 0, 60, 0, 0}}},  // week 1
    Week{Phase::Base, 2, {Session{Kind::Hills, 8, 45, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::Cruise, 4, 360, 0, 0}, Session{Kind::Long, 0, 65, 0, 0}}},  // week 2
    Week{Phase::Base, 2, {Session{Kind::Reps, 6, 400, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::Tempo, 0, 20, 0, 0}, Session{Kind::Long, 0, 70, 0, 0}}},  // week 3
    Week{Phase::CutBack, 3, {Session{Kind::EasyStrides, 0, 35, 0, 0}, Session{Kind::Easy, 0, 35, 0, 0}, Session{Kind::Easy, 0, 30, 0, 0}, Session{Kind::TimeTrial5k, 0, 0, 0, 0}}},  // week 4
    Week{Phase::Build, 0, {Session{Kind::Intervals, 5, 180, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::Cruise, 3, 480, 0, 0}, Session{Kind::Long, 0, 75, 0, 0}}},  // week 5
    Week{Phase::Build, 0, {Session{Kind::Intervals, 4, 240, 0, 0}, Session{Kind::EasyStrides, 0, 45, 0, 0}, Session{Kind::Cruise, 3, 600, 0, 0}, Session{Kind::Long, 0, 80, 0, 0}}},  // week 6
    Week{Phase::Build, 2, {Session{Kind::Intervals, 6, 180, 0, 0}, Session{Kind::EasyStrides, 0, 45, 0, 0}, Session{Kind::Pace10k, 4, 300, 0, 90}, Session{Kind::Long, 0, 85, 0, 0}}},  // week 7
    Week{Phase::CutBack, 3, {Session{Kind::EasyStrides, 0, 35, 0, 0}, Session{Kind::Easy, 0, 35, 0, 0}, Session{Kind::Easy, 0, 30, 0, 0}, Session{Kind::TimeTrial5k, 0, 0, 0, 0}}},  // week 8
    Week{Phase::Peak, 2, {Session{Kind::Intervals, 4, 240, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::Pace10k, 3, 540, 0, 120}, Session{Kind::Long, 0, 70, 0, 0}}},  // week 9
    Week{Phase::RaceWeek, 0, {Session{Kind::Pace10k, 4, 240, 0, 120}, Session{Kind::EasyStrides, 0, 30, 0, 0}, Session{Kind::Shakeout, 0, 0, 0, 0}, Session{Kind::Race, 0, 0, 0, 0}}},  // week 10
};
inline constexpr Week k10kTrialWeek = Week{Phase::CutBack, 3, {Session{Kind::EasyStrides, 0, 35, 0, 0}, Session{Kind::Easy, 0, 35, 0, 0}, Session{Kind::Easy, 0, 30, 0, 0}, Session{Kind::TimeTrial5k, 0, 0, 0, 0}}};

// half.plan
inline constexpr Week kHalfWeeks[] = {
    Week{Phase::Base, 2, {Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::Easy, 0, 35, 0, 0}, Session{Kind::Cruise, 3, 360, 0, 0}, Session{Kind::Long, 0, 70, 0, 0}}},  // week 1
    Week{Phase::Base, 2, {Session{Kind::Hills, 8, 45, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::Cruise, 4, 360, 0, 0}, Session{Kind::Long, 0, 75, 0, 0}}},  // week 2
    Week{Phase::Base, 2, {Session{Kind::Intervals, 5, 180, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::Tempo, 0, 20, 0, 0}, Session{Kind::Long, 0, 85, 0, 0}}},  // week 3
    Week{Phase::CutBack, 3, {Session{Kind::EasyStrides, 0, 35, 0, 0}, Session{Kind::Easy, 0, 35, 0, 0}, Session{Kind::Easy, 0, 30, 0, 0}, Session{Kind::TimeTrial5k, 0, 0, 0, 0}}},  // week 4
    Week{Phase::Build, 2, {Session{Kind::Intervals, 4, 240, 0, 0}, Session{Kind::EasyStrides, 0, 45, 0, 0}, Session{Kind::Cruise, 3, 600, 0, 0}, Session{Kind::Long, 0, 95, 0, 0}}},  // week 5
    Week{Phase::Build, 0, {Session{Kind::Cruise, 2, 900, 0, 0}, Session{Kind::Easy, 0, 45, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::LongFinish, 0, 100, 20, 0}}},  // week 6
    Week{Phase::Build, 2, {Session{Kind::Intervals, 6, 180, 0, 0}, Session{Kind::EasyStrides, 0, 45, 0, 0}, Session{Kind::PaceHalf, 3, 600, 0, 120}, Session{Kind::Long, 0, 105, 0, 0}}},  // week 7
    Week{Phase::CutBack, 3, {Session{Kind::EasyStrides, 0, 35, 0, 0}, Session{Kind::Easy, 0, 40, 0, 0}, Session{Kind::Easy, 0, 30, 0, 0}, Session{Kind::TimeTrial10k, 0, 0, 0, 0}}},  // week 8
    Week{Phase::Specific, 2, {Session{Kind::Cruise, 3, 600, 0, 0}, Session{Kind::EasyStrides, 0, 45, 0, 0}, Session{Kind::PaceHalf, 2, 1200, 0, 180}, Session{Kind::Long, 0, 115, 0, 0}}},  // week 9
    Week{Phase::Specific, 0, {Session{Kind::Tempo, 0, 20, 0, 0}, Session{Kind::Easy, 0, 45, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::LongFinish, 0, 100, 30, 0}}},  // week 10
    Week{Phase::Taper, 0, {Session{Kind::Cruise, 3, 480, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::PaceHalf, 2, 600, 0, 120}, Session{Kind::Long, 0, 75, 0, 0}}},  // week 11
    Week{Phase::RaceWeek, 0, {Session{Kind::PaceHalf, 3, 480, 0, 120}, Session{Kind::EasyStrides, 0, 30, 0, 0}, Session{Kind::Shakeout, 0, 0, 0, 0}, Session{Kind::Race, 0, 0, 0, 0}}},  // week 12
};
inline constexpr Week kHalfTrialWeek = Week{Phase::CutBack, 3, {Session{Kind::EasyStrides, 0, 35, 0, 0}, Session{Kind::Easy, 0, 35, 0, 0}, Session{Kind::Easy, 0, 30, 0, 0}, Session{Kind::TimeTrial5k, 0, 0, 0, 0}}};

// vo2max.plan
inline constexpr Week kVo2maxWeeks[] = {
    Week{Phase::Baseline, 3, {Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::Easy, 0, 35, 0, 0}, Session{Kind::ThirtyThirty, 2, 6, 0, 0}, Session{Kind::TimeTrial5k, 0, 0, 0, 0}}},  // week 1
    Week{Phase::Build, 0, {Session{Kind::Intervals, 5, 180, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::ThirtyThirty, 2, 8, 0, 0}, Session{Kind::Long, 0, 65, 0, 0}}},  // week 2
    Week{Phase::Build, 0, {Session{Kind::Intervals, 4, 240, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::Hills, 8, 60, 0, 0}, Session{Kind::Long, 0, 70, 0, 0}}},  // week 3
    Week{Phase::CutBack, 0, {Session{Kind::Intervals, 4, 180, 0, 0}, Session{Kind::Easy, 0, 35, 0, 0}, Session{Kind::EasyStrides, 0, 35, 0, 0}, Session{Kind::Long, 0, 55, 0, 0}}},  // week 4
    Week{Phase::Build, 0, {Session{Kind::Intervals, 6, 180, 0, 0}, Session{Kind::EasyStrides, 0, 45, 0, 0}, Session{Kind::ThirtyThirty, 2, 10, 0, 0}, Session{Kind::Long, 0, 75, 0, 0}}},  // week 5
    Week{Phase::Build, 0, {Session{Kind::Intervals, 4, 240, 0, 120}, Session{Kind::EasyStrides, 0, 45, 0, 0}, Session{Kind::Hills, 10, 60, 0, 0}, Session{Kind::Long, 0, 75, 0, 0}}},  // week 6
    Week{Phase::Peak, 0, {Session{Kind::Intervals, 6, 180, 0, 90}, Session{Kind::EasyStrides, 0, 45, 0, 0}, Session{Kind::ThirtyThirty, 3, 8, 0, 0}, Session{Kind::Long, 0, 80, 0, 0}}},  // week 7
    Week{Phase::Retest, 3, {Session{Kind::Intervals, 3, 180, 0, 0}, Session{Kind::Easy, 0, 30, 0, 0}, Session{Kind::Easy, 0, 25, 0, 0}, Session{Kind::TimeTrial5k, 0, 0, 0, 0}}},  // week 8
};
inline constexpr Week kVo2maxRecentWeek = Week{Phase::Baseline, 0, {Session{Kind::Intervals, 5, 120, 0, 0}, Session{Kind::EasyStrides, 0, 40, 0, 0}, Session{Kind::ThirtyThirty, 2, 6, 0, 0}, Session{Kind::Long, 0, 60, 0, 0}}};

inline constexpr PlanDef kPlans[] = {
    PlanDef{PlanId::TenK, "10K", 10, 6, 1000000u,
            {LongRange{45, 70}, LongRange{60, 85}, LongRange{70, 100}},
            {0, 0, 0},
            k10kWeeks, &k10kTrialWeek, nullptr},
    PlanDef{PlanId::Half, "Half marathon", 12, 8, 2109750u,
            {LongRange{60, 105}, LongRange{70, 115}, LongRange{80, 130}},
            {16000, 18000, 20000},
            kHalfWeeks, &kHalfTrialWeek, nullptr},
    PlanDef{PlanId::Vo2max, "VO2max block", 8, 8, 0u,
            {LongRange{45, 65}, LongRange{60, 80}, LongRange{70, 95}},
            {0, 0, 0},
            kVo2maxWeeks, nullptr, &kVo2maxRecentWeek},
};

inline constexpr uint8_t kPlanCount = sizeof(kPlans) / sizeof(kPlans[0]);

/// The plan with @p id, or null.
inline const PlanDef* find(PlanId id)
{
    for (const PlanDef& p : kPlans) {
        if (p.id == id) {
            return &p;
        }
    }
    return nullptr;
}

}  // namespace PlanData
}  // namespace XTraining

#endif  // XTRAINING_PLAN_DATA_HPP
