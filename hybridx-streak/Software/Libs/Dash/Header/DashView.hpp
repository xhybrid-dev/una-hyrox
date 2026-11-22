/**
 ******************************************************************************
 * @file    DashView.hpp
 * @brief   What the family dashboard shows (NOTES "S7"): plain data, no SDK
 *          types, trivially copyable, because it travels in a kernel message.
 *
 *   SportSum, WeekSum   the week by sport (WeekTotals.hpp fills them);
 *   FamilyView          the family's other figures (FamilyReader.hpp fills it):
 *                       X Training's plan for today, X Run's VO2max and
 *                       X Weigh-in's weight trend.
 *
 * Header-only: the GUI reads it, and the GUI links no Libs sources.
 ******************************************************************************
 */

#ifndef STREAK_DASH_VIEW_HPP
#define STREAK_DASH_VIEW_HPP

#include <cstdint>
#include <type_traits>

#include "StreakTypes.hpp"

namespace Streak
{

/// One kind's sum, or every kind's.
struct SportSum {
    uint32_t metres   = 0;   ///< 0 when no session of it had a distance
    uint16_t minutes  = 0;   ///< whole minutes of timer time, rounded down
    uint8_t  sessions = 0;
    uint8_t  pad      = 0;
};

/// A run of days by kind, and all kinds together.
struct WeekSum {
    SportSum kind[kKindCount] {};   ///< indexed by Streak::Kind
    SportSum all {};
};

/// X Training's plan today, as its glance would say it (TodayFile::pick).
struct PlanToday {
    static constexpr uint8_t kWhatLen = 20;   ///< XTraining::TodayDay::kWhatLen
    static constexpr uint8_t kPaceLen = 12;   ///< XTraining::TodayDay::kPaceLen

    uint8_t  state        = 0;   ///< XTraining::TodayFile::State; 0 = no file
    uint8_t  kind         = 0;   ///< today's XTraining::Kind
    uint8_t  hasNext      = 0;   ///< a session after today in the file
    uint8_t  nextTomorrow = 0;
    uint8_t  nextWeekday  = 0;   ///< 0 Monday .. 6 Sunday (X Training's order)
    uint8_t  nextKind     = 0;   ///< XTraining::Kind
    uint8_t  pad[2]       = {};
    uint32_t startDate    = 0;   ///< the plan's first day, yyyymmdd, for "Plan starts"
    char     what[kWhatLen]     = {};   ///< pre-worded by X Training: "4 x 4 min"
    char     pace[kPaceLen]     = {};   ///< "4:18/km"
    char     nextWhat[kWhatLen] = {};
};

/// X Run's VO2max (its vo2max.json).
struct Vo2Figure {
    uint16_t x10  = 0;   ///< 523 = 52.3
    uint8_t  runs = 0;   ///< the runs it is from
    uint8_t  pad  = 0;
};

/// X Weigh-in's trend (its Trend, from its shared weighins.csv).
struct WeightFigure {
    int32_t  trend         = 0;   ///< the trend at the last weigh-in, 0.01 kg
    int32_t  weekChange    = 0;   ///< trend(last) - trend(last - 7 days), 0.01 kg
    int32_t  lastDay       = 0;   ///< the last weigh-in's local day
    uint16_t days          = 0;   ///< days with a weigh-in
    uint8_t  hasWeekChange = 0;   ///< the log spans at least a week
    uint8_t  unit          = 0;   ///< XWeighIn::WeightUnit, as X Weigh-in shows it
};

struct FamilyView {
    static constexpr uint8_t kPlan   = 0x01;   ///< X Training's file read (any state but "no file")
    static constexpr uint8_t kVo2    = 0x02;   ///< X Run's figure read
    static constexpr uint8_t kWeight = 0x04;   ///< at least one weigh-in read

    uint8_t      flags   = 0;
    uint8_t      clockOk = 0;
    uint8_t      pad[2]  = {};
    int32_t      today   = 0;   ///< local day number the view is for
    PlanToday    plan {};
    Vo2Figure    vo2 {};
    WeightFigure weight {};
};

static_assert(std::is_trivially_copyable<WeekSum>::value, "WeekSum travels inside a kernel message");
static_assert(std::is_trivially_copyable<FamilyView>::value, "FamilyView travels inside a kernel message");

} // namespace Streak

#endif // STREAK_DASH_VIEW_HPP
