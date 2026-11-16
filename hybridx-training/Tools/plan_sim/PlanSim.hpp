/**
 * @file    PlanSim.hpp
 * @brief   Scenarios for the plan's memory (TP5), as Markdown: a runner's weeks with what went
 *          wrong in them, and what the plan (PlanRules) does about it, day by day.
 *
 * Host-only (std::string). Used by Tools/plan_sim and by the golden-file test, so what Jon
 * reviews at TP5's gate is exactly what the watch would do.
 */

#ifndef XTRAINING_PLAN_SIM_HPP
#define XTRAINING_PLAN_SIM_HPP

#include "Date.hpp"
#include "PlanTypes.hpp"
#include "WeekLayout.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace XTraining {

struct SimEvent {
    enum class Type : uint8_t {
        Miss,          ///< The day's session is not run.
        Part,          ///< The day's session is run, part done.
        Skip,          ///< The runner skips the day's session.
        DoTomorrow,    ///< Do tomorrow's session today.
        MoveTomorrow,  ///< Move today's session to tomorrow.
        AcceptOffer,
        DeclineOffer,
        Pause,
        Resume,
        Trial,         ///< The day's time trial or race is run in @p value seconds.
        AnswerKeep,    ///< Keep the paces (the question after a trial).
        AnswerUpdate,  ///< Update them.
    };
    Ymd      date = 0;
    Type     type = Type::Miss;
    uint32_t value = 0;
};

struct SimScenario {
    const char* title = "";
    const char* what = "";           ///< What the scenario shows, and the rule.
    PlanId      plan = PlanId::TenK;
    uint8_t     level = 2;
    uint32_t    fiveKSec = 22 * 60;
    DayMask     days = WeekLayout::kDefaultDays;
    uint8_t     longDay = WeekLayout::kDefaultLongDay;
    Ymd         start = 20270104u;   ///< A Monday.
    Ymd         raceDate = 0;
    Ymd         until = 0;           ///< The last day simulated.
    std::vector<SimEvent> events;
};

/// The Markdown of one scenario.
std::string runScenario(const SimScenario& s);

/// The scenarios Jon reviews.
std::vector<SimScenario> scenarios();

/// All of them, as one document.
std::string allScenarios();

}  // namespace XTraining

#endif  // XTRAINING_PLAN_SIM_HPP
