/**
 ******************************************************************************
 * @file    RidePlanText.hpp
 * @brief   The words for a ride's indoor plans: names, settings and their
 *          choices, step labels, the toast's heading, the summary's line.
 *          HybridX Workout's PlanText (W3), cut to Free, Intervals and
 *          Tabata. Pure C++, header-only (the GUI uses it), integer only;
 *          every writer fills the caller's buffer, never past it.
 ******************************************************************************
 */

#ifndef RIDE_RIDE_PLAN_TEXT_HPP
#define RIDE_RIDE_PLAN_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "PlanRunner.hpp"
#include "RidePlan.hpp"
#include "RideSnapshot.hpp"

namespace Ride {

namespace plantext {
inline size_t copyOut(char *out, size_t cap, const char *text)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    size_t n = 0;
    while (text[n] != '\0' && n + 1 < cap) {
        out[n] = text[n];
        ++n;
    }
    out[n] = '\0';
    return n;
}
} // namespace plantext

inline const char *kindName(SessionType t)
{
    switch (t) {
    case SessionType::Custom: return "Intervals";
    case SessionType::Tabata: return "Tabata";
    case SessionType::Plain:
    default:                  return "Free";
    }
}

constexpr SessionType nextKind(SessionType t)
{
    return static_cast<SessionType>((static_cast<uint8_t>(t) + 1u) % kSessionTypeCount);
}

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
    return plantext::copyOut(out, cap, tmp);
}

/// A countdown, rounded up: 00:01 until the step ends.
inline size_t formatCountdown(char *out, size_t cap, uint32_t remainingMs)
{
    const uint32_t up = remainingMs > UINT32_MAX - 999u ? UINT32_MAX : remainingMs + 999u;
    uint32_t s = up / 1000u;
    constexpr uint32_t kMaxS = 99u * 3600u + 59u * 60u + 59u;
    if (s > kMaxS) {
        s = kMaxS;
    }
    char tmp[16];
    if (s >= 3600u) {
        std::snprintf(tmp, sizeof(tmp), "%lu:%02lu:%02lu", static_cast<unsigned long>(s / 3600u),
                      static_cast<unsigned long>((s / 60u) % 60u), static_cast<unsigned long>(s % 60u));
    } else {
        std::snprintf(tmp, sizeof(tmp), "%02lu:%02lu", static_cast<unsigned long>(s / 60u),
                      static_cast<unsigned long>(s % 60u));
    }
    return plantext::copyOut(out, cap, tmp);
}

// --- Settings --------------------------------------------------------------------

enum class PlanField : uint8_t { Work, Rest, Rounds };

/// The settings a kind has, in screen order (Intervals: three; Free and Tabata: none).
inline uint8_t planFields(SessionType t, PlanField out[3])
{
    if (t != SessionType::Custom) {
        return 0;
    }
    out[0] = PlanField::Work;
    out[1] = PlanField::Rest;
    out[2] = PlanField::Rounds;
    return 3;
}

inline const char *fieldName(PlanField f)
{
    switch (f) {
    case PlanField::Work:   return "Work";
    case PlanField::Rest:   return "Rest";
    case PlanField::Rounds: return "Rounds";
    }
    return "";
}

/// The choices R1 steps through (seconds, or a count), as Workout's.
inline const uint16_t *choicesOf(PlanField f, uint8_t &n)
{
    static constexpr uint16_t kWork[]   = {10, 15, 20, 30, 40, 45, 60, 90, 120, 180, 240, 300};
    static constexpr uint16_t kRest[]   = {5, 10, 15, 20, 30, 45, 60, 90, 120};
    static constexpr uint16_t kRounds[] = {1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30};
    switch (f) {
    case PlanField::Work:   n = static_cast<uint8_t>(sizeof(kWork) / sizeof(kWork[0])); return kWork;
    case PlanField::Rest:   n = static_cast<uint8_t>(sizeof(kRest) / sizeof(kRest[0])); return kRest;
    case PlanField::Rounds: n = static_cast<uint8_t>(sizeof(kRounds) / sizeof(kRounds[0])); return kRounds;
    }
    n = 0;
    return nullptr;
}

/// The choice after @p current: the first above it, round to the first.
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

inline uint16_t fieldValue(const PlanConfig &c, PlanField f)
{
    switch (f) {
    case PlanField::Work:   return c.workSec;
    case PlanField::Rest:   return c.restSec;
    case PlanField::Rounds: return c.rounds;
    }
    return 0;
}

inline PlanConfig stepField(PlanConfig c, PlanField f)
{
    uint8_t n = 0;
    const uint16_t *list = choicesOf(f, n);
    const uint16_t v = nextChoice(list, n, fieldValue(c, f));
    switch (f) {
    case PlanField::Work:   c.workSec = v; break;
    case PlanField::Rest:   c.restSec = v; break;
    case PlanField::Rounds: c.rounds = static_cast<uint8_t>(v); break;
    }
    return clamp(c);
}

inline size_t formatField(char *out, size_t cap, const PlanConfig &c, PlanField f)
{
    if (f == PlanField::Rounds) {
        char tmp[8];
        std::snprintf(tmp, sizeof(tmp), "%u", static_cast<unsigned>(c.rounds));
        return plantext::copyOut(out, cap, tmp);
    }
    return formatSeconds(out, cap, fieldValue(c, f));
}

/// The plan in a line, for the main menu: "40 s / 20 s x 6", "8 x 20 s / 10 s". Empty for Free.
inline size_t formatPlanSummary(char *out, size_t cap, const PlanConfig &raw)
{
    const PlanConfig c = clamp(raw);
    char a[16];
    char b[16];
    char tmp[40];
    switch (c.type) {
    case SessionType::Custom:
        formatSeconds(a, sizeof(a), c.workSec);
        formatSeconds(b, sizeof(b), c.restSec);
        std::snprintf(tmp, sizeof(tmp), "%s / %s x %u", a, b, static_cast<unsigned>(c.rounds));
        break;
    case SessionType::Tabata:
        std::snprintf(tmp, sizeof(tmp), "%u x %u s / %u s", static_cast<unsigned>(kTabataRounds),
                      static_cast<unsigned>(kTabataWorkSec), static_cast<unsigned>(kTabataRestSec));
        break;
    case SessionType::Plain:
    default:
        tmp[0] = '\0';
        break;
    }
    return plantext::copyOut(out, cap, tmp);
}

// --- Steps on the screens -----------------------------------------------------------

/// A lap's short name: "Work 3", "Rest 3"; a free ride's "Lap 3" or "Auto 3".
inline size_t formatStepName(char *out, size_t cap, uint8_t type, uint8_t kind, uint8_t round, uint8_t lapNo,
                             bool autoEnded)
{
    char tmp[16];
    if (static_cast<SessionType>(type) == SessionType::Plain) {
        std::snprintf(tmp, sizeof(tmp), "%s %u", autoEnded ? "Auto" : "Lap", static_cast<unsigned>(lapNo));
    } else {
        std::snprintf(tmp, sizeof(tmp), "%s %u", kind == static_cast<uint8_t>(StepKind::Rest) ? "Rest" : "Work",
                      static_cast<unsigned>(round));
    }
    return plantext::copyOut(out, cap, tmp);
}

/// The live heading: "Work 3/8", "Rest 3/8", "Done"; empty for a free ride.
inline size_t formatStepHeading(char *out, size_t cap, const PlanView &v)
{
    if (v.steps == 0) {
        return plantext::copyOut(out, cap, "");
    }
    if (v.done) {
        return plantext::copyOut(out, cap, "Done");
    }
    char tmp[16];
    std::snprintf(tmp, sizeof(tmp), "%s %u/%u", v.kind == static_cast<uint8_t>(StepKind::Rest) ? "Rest" : "Work",
                  static_cast<unsigned>(v.round), static_cast<unsigned>(v.rounds));
    return plantext::copyOut(out, cap, tmp);
}

/// Every step of a ride's plan is timed, so the big number counts down while a plan runs.
inline bool countsDown(const PlanView &v)
{
    return v.steps > 0 && !v.done && v.remainingMs > 0;
}

/// The toast's heading: what begins now ("Work 4", "Rest", "Done"; a free ride's
/// "Lap 4" or "Auto lap"). From the cue and the lap just finished: the toast
/// arrives before the snapshot of the new step.
inline size_t formatCueHeading(char *out, size_t cap, const LapEventData &e)
{
    char tmp[16];
    switch (static_cast<Cue>(e.cue)) {
    case Cue::Done: return plantext::copyOut(out, cap, "Done");
    case Cue::Rest: return plantext::copyOut(out, cap, "Rest");
    case Cue::Work:
        std::snprintf(tmp, sizeof(tmp), "Work %u", static_cast<unsigned>(e.round) + 1u);
        return plantext::copyOut(out, cap, tmp);
    default:
        if (e.autoEnded) {
            return plantext::copyOut(out, cap, "Auto lap");
        }
        std::snprintf(tmp, sizeof(tmp), "Lap %u", static_cast<unsigned>(e.finishedLap) + 1u);
        return plantext::copyOut(out, cap, tmp);
    }
}

/// The summary's line: "6 laps", "6 rounds", "8 of 8 rounds" (Tabata).
inline size_t formatSummaryLine(char *out, size_t cap, uint8_t type, uint8_t laps)
{
    char tmp[24];
    const unsigned rounds = (laps + 1u) / 2u;
    switch (static_cast<SessionType>(type)) {
    case SessionType::Custom:
        std::snprintf(tmp, sizeof(tmp), "%u %s", rounds, rounds == 1 ? "round" : "rounds");
        break;
    case SessionType::Tabata:
        std::snprintf(tmp, sizeof(tmp), "%u of %u rounds", rounds, static_cast<unsigned>(kTabataRounds));
        break;
    case SessionType::Plain:
    default:
        std::snprintf(tmp, sizeof(tmp), "%u %s", static_cast<unsigned>(laps), laps == 1 ? "lap" : "laps");
        break;
    }
    return plantext::copyOut(out, cap, tmp);
}

} // namespace Ride

#endif // RIDE_RIDE_PLAN_TEXT_HPP
