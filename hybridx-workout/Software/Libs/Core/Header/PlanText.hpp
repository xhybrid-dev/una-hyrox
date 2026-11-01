/**
 ******************************************************************************
 * @file    PlanText.hpp
 * @brief   The words the screens use for the kinds of workout (W3): names,
 *          the settings and their choices, each step's label, the lap toast's
 *          heading and the summary's line. Pure C++, header-only (the GUI uses
 *          it), integer only. Every writer fills the caller's buffer, never
 *          past it, always terminates it (cap > 0) and returns its length.
 *
 * Widths. The round face is narrow at the top, so the live heading is short
 * ("Set 2/3", "Min 4/10", "Sled push"); the simulator's text-fit audit
 * measures each with the real font (docs/experiments/audit.sh).
 *
 * Choices. Each setting steps through a short list of sensible values on R1
 * (choicesOf, nextChoice), the way the auto lap does, rather than one at a
 * time: no list is longer than 20. Every value is inside its range in
 * SessionPlan.hpp; a host test checks each list against it.
 ******************************************************************************
 */

#ifndef WORKOUT_PLAN_TEXT_HPP
#define WORKOUT_PLAN_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "PlanRunner.hpp"
#include "SessionPlan.hpp"
#include "WorkoutSnapshot.hpp"
#include "WorkoutText.hpp"

namespace Workout {

/// What each kind is called on the screens.
inline const char *kindName(SessionType t)
{
    switch (t) {
    case SessionType::Strength: return "Strength";
    case SessionType::Emom:     return "EMOM";
    case SessionType::Amrap:    return "AMRAP";
    case SessionType::Tabata:   return "Tabata";
    case SessionType::Custom:   return "Intervals";
    case SessionType::Circuit:  return "Circuit";
    case SessionType::Plain:
    default:                    return "Free";
    }
}

/// The kind after @p t, round to Free.
constexpr SessionType nextKind(SessionType t)
{
    return static_cast<SessionType>((static_cast<uint8_t>(t) + 1u) % kSessionTypeCount);
}

/// The stations as the screens name them: mixed case and short, so a lap row
/// and the live heading fit (Race's own names are upper case and longer).
constexpr const char *kStationShort[kCircuitStationCount] = {
    "SkiErg", "Sled push", "Sled pull", "Burpees", "Row", "Carry", "Lunges", "Wall balls",
};

inline const char *stationShort(uint8_t n)
{
    return kStationShort[(n >= 1 && n <= kCircuitStationCount) ? n - 1 : 0];
}

// --- Durations and distances ---------------------------------------------------

/// Seconds as a setting shows them: "45 s" under two minutes, else "2 min" or "2:30".
inline size_t formatSeconds(char *out, size_t cap, uint32_t sec)
{
    char tmp[16];
    if (sec < 120u) {
        std::snprintf(tmp, sizeof(tmp), "%lu s", static_cast<unsigned long>(sec));
    } else if (sec % 60u == 0) {
        std::snprintf(tmp, sizeof(tmp), "%lu min", static_cast<unsigned long>(sec / 60u));
    } else {
        std::snprintf(tmp, sizeof(tmp), "%lu:%02lu", static_cast<unsigned long>(sec / 60u),
                      static_cast<unsigned long>(sec % 60u));
    }
    return detail::copyOut(out, cap, tmp);
}

/// A run's length: "1 km", or "500 m".
inline size_t formatRunM(char *out, size_t cap, uint16_t m)
{
    char tmp[12];
    if (m >= 1000u && m % 1000u == 0) {
        std::snprintf(tmp, sizeof(tmp), "%u km", static_cast<unsigned>(m / 1000u));
    } else {
        std::snprintf(tmp, sizeof(tmp), "%u m", static_cast<unsigned>(m));
    }
    return detail::copyOut(out, cap, tmp);
}

/// A countdown's seconds, rounded up, so it reads 00:01 until the step ends.
inline size_t formatCountdown(char *out, size_t cap, uint32_t remainingMs)
{
    const uint32_t up = remainingMs > UINT32_MAX - 999u ? UINT32_MAX : remainingMs + 999u;
    return formatClock(out, cap, up / 1000u * 1000u);
}

// --- The settings --------------------------------------------------------------

enum class PlanField : uint8_t { Sets, SetRest, SetCap, EmomMin, AmrapMin, Work, Rest, Rounds, CircuitRounds, RunM };

/// The settings a kind has, in screen order. Returns how many (at most 3).
inline uint8_t planFields(SessionType t, PlanField out[3])
{
    switch (t) {
    case SessionType::Strength:
        out[0] = PlanField::Sets; out[1] = PlanField::SetRest; out[2] = PlanField::SetCap;
        return 3;
    case SessionType::Emom:   out[0] = PlanField::EmomMin; return 1;
    case SessionType::Amrap:  out[0] = PlanField::AmrapMin; return 1;
    case SessionType::Custom:
        out[0] = PlanField::Work; out[1] = PlanField::Rest; out[2] = PlanField::Rounds;
        return 3;
    case SessionType::Circuit:
        out[0] = PlanField::CircuitRounds; out[1] = PlanField::RunM;
        return 2;
    case SessionType::Tabata:   // fixed: 8 x 20/10
    case SessionType::Plain:
    default:
        return 0;
    }
}

inline const char *fieldName(PlanField f)
{
    switch (f) {
    case PlanField::Sets:          return "Sets";
    case PlanField::SetRest:       return "Rest";
    case PlanField::SetCap:        return "Set cap";
    case PlanField::EmomMin:       return "Minutes";
    case PlanField::AmrapMin:      return "Time cap";
    case PlanField::Work:          return "Work";
    case PlanField::Rest:          return "Rest";
    case PlanField::Rounds:        return "Rounds";
    case PlanField::CircuitRounds: return "Rounds";
    case PlanField::RunM:          return "Run";
    }
    return "";
}

/// The choices R1 steps through, in the field's own unit (seconds, minutes,
/// metres or a count). @p n gets how many.
inline const uint16_t *choicesOf(PlanField f, uint8_t &n)
{
    static constexpr uint16_t kSets[]     = {1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20};
    static constexpr uint16_t kSetRest[]  = {15, 30, 45, 60, 90, 120, 180, 240, 300};
    static constexpr uint16_t kSetCap[]   = {60, 120, 180, 300, 600, 900};
    static constexpr uint16_t kMinutes[]  = {5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 45, 60};
    static constexpr uint16_t kWork[]     = {10, 15, 20, 30, 40, 45, 60, 90, 120, 180, 240, 300};
    static constexpr uint16_t kRest[]     = {5, 10, 15, 20, 30, 45, 60, 90, 120};
    static constexpr uint16_t kRounds[]   = {1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30};
    static constexpr uint16_t kCRounds[]  = {1, 2, 3, 4, 5, 6, 7, 8};
    static constexpr uint16_t kRunM[]     = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
#define WORKOUT_CHOICES(a) n = static_cast<uint8_t>(sizeof(a) / sizeof(a[0])); return a
    switch (f) {
    case PlanField::Sets:          WORKOUT_CHOICES(kSets);
    case PlanField::SetRest:       WORKOUT_CHOICES(kSetRest);
    case PlanField::SetCap:        WORKOUT_CHOICES(kSetCap);
    case PlanField::EmomMin:       WORKOUT_CHOICES(kMinutes);
    case PlanField::AmrapMin:      WORKOUT_CHOICES(kMinutes);
    case PlanField::Work:          WORKOUT_CHOICES(kWork);
    case PlanField::Rest:          WORKOUT_CHOICES(kRest);
    case PlanField::Rounds:        WORKOUT_CHOICES(kRounds);
    case PlanField::CircuitRounds: WORKOUT_CHOICES(kCRounds);
    case PlanField::RunM:          WORKOUT_CHOICES(kRunM);
    }
#undef WORKOUT_CHOICES
    n = 0;
    return nullptr;
}

/// The choice after @p current: the first one above it, round to the first.
/// A value not in the list (set by an older file) moves to the next one up.
inline uint16_t nextChoice(const uint16_t *list, uint8_t n, uint16_t current)
{
    if (list == nullptr || n == 0) {
        return current;
    }
    for (uint8_t i = 0; i < n; ++i) {
        if (list[i] > current) {
            return list[i];
        }
    }
    return list[0];
}

/// A field's value, in its own unit.
inline uint16_t fieldValue(const PlanConfig &c, PlanField f)
{
    switch (f) {
    case PlanField::Sets:          return c.sets;
    case PlanField::SetRest:       return c.setRestSec;
    case PlanField::SetCap:        return c.setCapSec;
    case PlanField::EmomMin:       return c.emomMin;
    case PlanField::AmrapMin:      return c.amrapMin;
    case PlanField::Work:          return c.workSec;
    case PlanField::Rest:          return c.restSec;
    case PlanField::Rounds:        return c.rounds;
    case PlanField::CircuitRounds: return c.circuitRounds;
    case PlanField::RunM:          return c.runM;
    }
    return 0;
}

/// @p c with field @p f stepped to its next choice, clamped.
inline PlanConfig stepField(PlanConfig c, PlanField f)
{
    uint8_t n = 0;
    const uint16_t *list = choicesOf(f, n);
    const uint16_t v = nextChoice(list, n, fieldValue(c, f));
    switch (f) {
    case PlanField::Sets:          c.sets = static_cast<uint8_t>(v); break;
    case PlanField::SetRest:       c.setRestSec = v; break;
    case PlanField::SetCap:        c.setCapSec = v; break;
    case PlanField::EmomMin:       c.emomMin = static_cast<uint8_t>(v); break;
    case PlanField::AmrapMin:      c.amrapMin = static_cast<uint8_t>(v); break;
    case PlanField::Work:          c.workSec = v; break;
    case PlanField::Rest:          c.restSec = v; break;
    case PlanField::Rounds:        c.rounds = static_cast<uint8_t>(v); break;
    case PlanField::CircuitRounds: c.circuitRounds = static_cast<uint8_t>(v); break;
    case PlanField::RunM:          c.runM = v; break;
    }
    return clamp(c);
}

/// A field's value as its hint shows it: "3", "60 s", "5 min", "1 km".
inline size_t formatField(char *out, size_t cap, const PlanConfig &c, PlanField f)
{
    const uint16_t v = fieldValue(c, f);
    char tmp[16];
    switch (f) {
    case PlanField::SetRest:
    case PlanField::SetCap:
    case PlanField::Work:
    case PlanField::Rest:
        return formatSeconds(out, cap, v);
    case PlanField::EmomMin:
    case PlanField::AmrapMin:
        std::snprintf(tmp, sizeof(tmp), "%u min", static_cast<unsigned>(v));
        return detail::copyOut(out, cap, tmp);
    case PlanField::RunM:
        return formatRunM(out, cap, v);
    default:
        std::snprintf(tmp, sizeof(tmp), "%u", static_cast<unsigned>(v));
        return detail::copyOut(out, cap, tmp);
    }
}

/// The plan in a line, for the main menu's hint: "3 sets, 60 s rest", "10 min",
/// "12 min cap", "8 x 20 s / 10 s", "40 s / 20 s x 6", "8 rounds, 1 km". Empty for Free.
inline size_t formatPlanSummary(char *out, size_t cap, const PlanConfig &raw)
{
    const PlanConfig c = clamp(raw);
    char a[16];
    char b[16];
    char tmp[40];
    switch (c.type) {
    case SessionType::Strength:
        formatSeconds(a, sizeof(a), c.setRestSec);
        std::snprintf(tmp, sizeof(tmp), "%u %s, %s rest", static_cast<unsigned>(c.sets),
                      c.sets == 1 ? "set" : "sets", a);
        break;
    case SessionType::Emom:
        std::snprintf(tmp, sizeof(tmp), "%u min", static_cast<unsigned>(c.emomMin));
        break;
    case SessionType::Amrap:
        std::snprintf(tmp, sizeof(tmp), "%u min cap", static_cast<unsigned>(c.amrapMin));
        break;
    case SessionType::Tabata:
        std::snprintf(tmp, sizeof(tmp), "%u x %u s / %u s", static_cast<unsigned>(kTabataRounds),
                      static_cast<unsigned>(kTabataWorkSec), static_cast<unsigned>(kTabataRestSec));
        break;
    case SessionType::Custom:
        formatSeconds(a, sizeof(a), c.workSec);
        formatSeconds(b, sizeof(b), c.restSec);
        std::snprintf(tmp, sizeof(tmp), "%s / %s x %u", a, b, static_cast<unsigned>(c.rounds));
        break;
    case SessionType::Circuit:
        formatRunM(a, sizeof(a), c.runM);
        std::snprintf(tmp, sizeof(tmp), "%u %s, %s", static_cast<unsigned>(c.circuitRounds),
                      c.circuitRounds == 1 ? "round" : "rounds", a);
        break;
    case SessionType::Plain:
    default:
        tmp[0] = '\0';
        break;
    }
    return detail::copyOut(out, cap, tmp);
}

// --- Steps on the screens ------------------------------------------------------

/// A step's short name, for a lap row and the toast: "Set 2", "Rest 2", "Min 3",
/// "Round 3", "Work 3", "Run 1", "Sled push". A Free workout's lap is "Lap 3", or
/// "Auto 3" when the auto lap ended it (@p lapNo, from 1).
inline size_t formatStepName(char *out, size_t cap, uint8_t type, uint8_t kind, uint8_t round,
                             uint8_t station, uint8_t lapNo, bool autoEnded)
{
    char tmp[24];
    const auto t = static_cast<SessionType>(type);
    const auto k = static_cast<StepKind>(kind);
    const unsigned r = round;
    switch (t) {
    case SessionType::Strength:
        std::snprintf(tmp, sizeof(tmp), "%s %u", k == StepKind::Rest ? "Rest" : "Set", r);
        break;
    case SessionType::Emom:
        std::snprintf(tmp, sizeof(tmp), "Min %u", r);
        break;
    case SessionType::Amrap:
        std::snprintf(tmp, sizeof(tmp), "Round %u", r);
        break;
    case SessionType::Tabata:
    case SessionType::Custom:
        std::snprintf(tmp, sizeof(tmp), "%s %u", k == StepKind::Rest ? "Rest" : "Work", r);
        break;
    case SessionType::Circuit:
        if (k == StepKind::Station) {
            return detail::copyOut(out, cap, stationShort(station));
        }
        std::snprintf(tmp, sizeof(tmp), "Run %u", r);
        break;
    case SessionType::Plain:
    default:
        std::snprintf(tmp, sizeof(tmp), "%s %u", autoEnded ? "Auto" : "Lap", static_cast<unsigned>(lapNo));
        break;
    }
    return detail::copyOut(out, cap, tmp);
}

/// The live screen's heading for the step under way: "Set 2/3", "Rest",
/// "Min 4/10", "Round 4", "Work 3/8", "Rest 3/8", "Run 2/8", "Sled push", or
/// "Done" once the plan is over. Empty for a Free workout (its face shows the lap).
inline size_t formatStepHeading(char *out, size_t cap, const PlanView &v)
{
    char tmp[24];
    const auto t = static_cast<SessionType>(v.type);
    const auto k = static_cast<StepKind>(v.kind);
    if (v.steps == 0) {
        return detail::copyOut(out, cap, "");
    }
    if (v.done) {
        return detail::copyOut(out, cap, "Done");
    }
    switch (t) {
    case SessionType::Strength:
        if (k == StepKind::Rest) {
            return detail::copyOut(out, cap, "Rest");
        }
        std::snprintf(tmp, sizeof(tmp), "Set %u/%u", static_cast<unsigned>(v.round), static_cast<unsigned>(v.rounds));
        break;
    case SessionType::Emom:
        std::snprintf(tmp, sizeof(tmp), "Min %u/%u", static_cast<unsigned>(v.round), static_cast<unsigned>(v.rounds));
        break;
    case SessionType::Amrap:
        std::snprintf(tmp, sizeof(tmp), "Round %u", static_cast<unsigned>(v.round));
        break;
    case SessionType::Tabata:
    case SessionType::Custom:
        std::snprintf(tmp, sizeof(tmp), "%s %u/%u", k == StepKind::Rest ? "Rest" : "Work",
                      static_cast<unsigned>(v.round), static_cast<unsigned>(v.rounds));
        break;
    case SessionType::Circuit:
        if (k == StepKind::Station) {
            return detail::copyOut(out, cap, stationShort(v.station));
        }
        std::snprintf(tmp, sizeof(tmp), "Run %u/%u", static_cast<unsigned>(v.round), static_cast<unsigned>(v.rounds));
        break;
    default:
        tmp[0] = '\0';
        break;
    }
    return detail::copyOut(out, cap, tmp);
}

/// True when the live screen's big number is a countdown: a timed step (a
/// strength set counts up: its cap is a backstop, not a target).
inline bool countsDown(const PlanView &v)
{
    if (v.steps == 0 || v.done || v.remainingMs == 0) {
        return false;
    }
    return !(static_cast<SessionType>(v.type) == SessionType::Strength &&
             static_cast<StepKind>(v.kind) == StepKind::Work);
}

/// True when R2 does something for this kind: the timers ignore it.
inline bool buttonCounts(uint8_t type)
{
    const auto t = static_cast<SessionType>(type);
    return t != SessionType::Emom && t != SessionType::Tabata && t != SessionType::Custom;
}

/// The lap toast's heading: what begins now, from the cue and the lap just
/// finished (the toast arrives before the snapshot of the new step). "Rest",
/// "Set 3", "Minute 5", "Round 4", "Work 3", "Run 2", "Sled push", "Done";
/// for a Free workout "Lap 4", or "Auto lap".
inline size_t formatCueHeading(char *out, size_t cap, uint8_t type, const LapEventData &e)
{
    char tmp[24];
    const auto t = static_cast<SessionType>(type);
    const unsigned next = static_cast<unsigned>(e.round) + 1u;
    switch (static_cast<Cue>(e.cue)) {
    case Cue::Done:
        return detail::copyOut(out, cap, "Done");
    case Cue::Rest:
        return detail::copyOut(out, cap, "Rest");
    case Cue::Run:
        std::snprintf(tmp, sizeof(tmp), "Run %u", next);
        break;
    case Cue::Station:
        return detail::copyOut(out, cap, stationShort(e.round));   // run r is followed by station r
    case Cue::Work:
        // After rest r comes set or work r + 1; after EMOM minute r, minute r + 1.
        std::snprintf(tmp, sizeof(tmp), "%s %u",
                      t == SessionType::Strength ? "Set" : (t == SessionType::Emom ? "Minute" : "Work"), next);
        break;
    case Cue::Round:
    default:
        if (t == SessionType::Amrap) {
            std::snprintf(tmp, sizeof(tmp), "Round %u", next);
        } else if (e.autoEnded) {
            std::snprintf(tmp, sizeof(tmp), "Auto lap");
        } else {
            std::snprintf(tmp, sizeof(tmp), "Lap %u", static_cast<unsigned>(e.finishedLap) + 1u);
        }
        break;
    }
    return detail::copyOut(out, cap, tmp);
}

/// The summary's line under the time: "4 laps", "3 sets", "10 minutes",
/// "7 rounds", "8 of 8 rounds" (Tabata), "6 rounds", "8 stations".
inline size_t formatSummaryLine(char *out, size_t cap, uint8_t type, uint8_t laps, uint16_t amrapRounds)
{
    char tmp[32];
    unsigned n = laps;
    const char *one = "lap";
    const char *many = "laps";
    switch (static_cast<SessionType>(type)) {
    case SessionType::Strength: n = (laps + 1u) / 2u; one = "set"; many = "sets"; break;
    case SessionType::Emom:     one = "minute"; many = "minutes"; break;
    case SessionType::Amrap:    n = amrapRounds; one = "round"; many = "rounds"; break;
    case SessionType::Tabata:
        std::snprintf(tmp, sizeof(tmp), "%u of %u rounds", (laps + 1u) / 2u, static_cast<unsigned>(kTabataRounds));
        return detail::copyOut(out, cap, tmp);
    case SessionType::Custom:   n = (laps + 1u) / 2u; one = "round"; many = "rounds"; break;
    case SessionType::Circuit:  n = laps / 2u; one = "station"; many = "stations"; break;
    case SessionType::Plain:
    default:
        break;
    }
    std::snprintf(tmp, sizeof(tmp), "%u %s", n, n == 1 ? one : many);
    return detail::copyOut(out, cap, tmp);
}

} // namespace Workout

#endif // WORKOUT_PLAN_TEXT_HPP
