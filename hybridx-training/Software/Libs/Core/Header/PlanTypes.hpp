/**
 ******************************************************************************
 * @file    PlanTypes.hpp
 * @brief   What a plan is made of: sessions, weeks and plans, as the plan
 *          files describe them (the .plan files in Tools/Plans; PLANS.md sections 3-7).
 *
 * Header-only: the generated data/PlanData.hpp holds the plans as constexpr
 * tables of these, and the GUI as well as the service reads them.
 *
 * A plan file is written for level 2; PlanBuilder makes levels 1 and 3 from
 * it (PLANS.md section 4.1) and applies the caps (section 3.3).
 ******************************************************************************
 */

#ifndef XTRAINING_PLAN_TYPES_HPP
#define XTRAINING_PLAN_TYPES_HPP

#include <cstdint>

namespace XTraining {

/// The kinds of session (PLANS.md section 3.1).
enum class Kind : uint8_t {
    Rest = 0,
    Easy,          ///< a = minutes.
    EasyStrides,   ///< a = minutes, then 6 strides.
    Long,          ///< a = minutes.
    LongFinish,    ///< a = minutes in all, b = minutes at half pace at the end. A hard day.
    Tempo,         ///< a = minutes at threshold.
    Cruise,        ///< reps x a seconds at threshold.
    Intervals,     ///< reps x a seconds at interval pace.
    ThirtyThirty,  ///< reps sets of a x (30 s at interval pace, 30 s easy).
    Hills,         ///< reps x a seconds uphill.
    Reps,          ///< reps x a metres at repetition pace.
    Pace10k,       ///< reps x a seconds at 10K pace.
    PaceHalf,      ///< reps x a seconds at half pace.
    TimeTrial5k,
    TimeTrial10k,
    Shakeout,
    Race,
};

constexpr uint8_t kKindCount = static_cast<uint8_t>(Kind::Race) + 1;

/// One session as a plan file gives it. jogS 0 means the default recovery
/// for the kind (PlanBuilder).
struct Session {
    Kind     kind = Kind::Rest;
    uint8_t  reps = 0;
    uint16_t a    = 0;
    uint16_t b    = 0;
    uint16_t jogS = 0;
};

/// A hard day: quality, a time trial, a race, or a long run with a fast finish.
constexpr bool isHard(Kind k)
{
    switch (k) {
    case Kind::Rest:
    case Kind::Easy:
    case Kind::EasyStrides:
    case Kind::Long:
    case Kind::Shakeout:
        return false;
    default:
        return true;
    }
}

enum class Phase : uint8_t { Base, Build, Peak, Specific, Taper, RaceWeek, CutBack, Baseline, Retest };

/// The four slots of a week in a plan file: the columns of PLANS.md's tables.
enum Slot : uint8_t {
    kSlotQ1   = 0,  ///< First quality (the Tuesday column).
    kSlotEasy = 1,  ///< The easy run (Wednesday).
    kSlotQ2   = 2,  ///< Second quality (Friday).
    kSlotLong = 3,  ///< The long run (Sunday).
    kSlotCount = 4,
};

struct Week {
    Phase   phase = Phase::Base;
    uint8_t key   = 0;  ///< The slot holding the week's key session.
    Session slot[kSlotCount];
};

enum class PlanId : uint8_t { None = 0, TenK = 1, Half = 2, Vo2max = 3 };

constexpr uint8_t kLevels = 3;

/// A level's long-run range (start of the plan to its peak), in minutes.
struct LongRange {
    uint16_t start = 0;
    uint16_t peak  = 0;
};

struct PlanDef {
    PlanId      id = PlanId::None;
    const char* name = "";        ///< "10K", "Half marathon", "VO2max block".
    uint8_t     weekCount = 0;
    uint8_t     minWeeks  = 0;    ///< The fewest weeks it may be cut to before a race.
    uint32_t    raceCm    = 0;    ///< The race distance; 0 for a block without a race.
    LongRange   longRange[kLevels];      ///< Levels 1-3; index 1 is the plan file's own.
    uint32_t    longFloorM[kLevels];     ///< The peak long run's distance floor; 0 for none.
    const Week* weeks = nullptr;
    const Week* trialWeek  = nullptr;  ///< Week 1 when the 5K is not recent; null to keep week 1.
    const Week* recentWeek = nullptr;  ///< Week 1 when it is; null to keep week 1.
};

}  // namespace XTraining

#endif  // XTRAINING_PLAN_TYPES_HPP
