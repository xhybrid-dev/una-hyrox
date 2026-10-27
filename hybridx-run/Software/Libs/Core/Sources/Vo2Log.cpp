/**
 ******************************************************************************
 * @file    Vo2Log.cpp
 * @brief   The tuning log's text (see the header).
 ******************************************************************************
 */

#include "Vo2Log.hpp"

#include <cstdarg>
#include <cstdio>

#include "Vo2Text.hpp"

namespace RunVo2::Log
{

namespace
{

/// snprintf that reports 0 (and leaves "") when the line does not fit.
size_t put(char* buf, size_t cap, const char* fmt, ...)
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    va_list ap;
    va_start(ap, fmt);
    const int n = std::vsnprintf(buf, cap, fmt, ap);
    va_end(ap);
    if (n < 0 || static_cast<size_t>(n) >= cap) {
        buf[0] = '\0';
        return 0;
    }
    return static_cast<size_t>(n);
}

/// "12.3" or "-0.4" from a signed x10 value.
struct X10 {
    char s[12];
    explicit X10(int32_t v)
    {
        const uint32_t a = static_cast<uint32_t>(v < 0 ? -v : v);
        std::snprintf(s, sizeof(s), "%s%lu.%lu", v < 0 ? "-" : "",
                      static_cast<unsigned long>(a / 10), static_cast<unsigned long>(a % 10));
    }
};

/// "1.034" from an x1000 value.
struct X1000 {
    char s[12];
    explicit X1000(uint32_t v)
    {
        std::snprintf(s, sizeof(s), "%lu.%03lu", static_cast<unsigned long>(v / 1000),
                      static_cast<unsigned long>(v % 1000));
    }
};

}  // namespace

const char* outcomeName(WindowOutcome o)
{
    switch (o) {
    case WindowOutcome::Kept:     return "kept";
    case WindowOutcome::WarmUp:   return "warmup";
    case WindowOutcome::Gaps:     return "gaps";
    case WindowOutcome::Unsteady: return "unsteady";
    case WindowOutcome::Grade:    return "grade";
    case WindowOutcome::Overflow: return "overflow";
    case WindowOutcome::Lagged:   return "lagged";
    }
    return "?";
}

const char* statusName(RunStatus s)
{
    switch (s) {
    case RunStatus::Ok:                return "ok";
    case RunStatus::ProfileIncomplete: return "profile";
    case RunStatus::NotEnoughRunning:  return "not_enough";
    case RunStatus::TooEasy:           return "too_easy";
    case RunStatus::HrAboveMax:        return "above_max";
    }
    return "?";
}

namespace
{

const char* profileName(ProfileStatus s)
{
    switch (s) {
    case ProfileStatus::Ok:              return "ok";
    case ProfileStatus::NeedsAge:        return "needs_age";
    case ProfileStatus::NeedsRestingHr:  return "needs_rest_hr";
    case ProfileStatus::ReserveTooSmall: return "reserve_too_small";
    }
    return "?";
}

const char* orNone(const char* s)
{
    return (s != nullptr && s[0] != '\0') ? s : "none";
}

}  // namespace

size_t contextLine(std::time_t startUtc, const Profile& p, char* buf, size_t cap)
{
    const Civil c = civilFromUtc(startUtc);
    return put(buf, cap,
               "# HybridX Run VO2 log 1, start %04ld-%02u-%02uT%02u:%02u:%02uZ (%lu), "
               "max_hr %u (%s), rest_hr %u (%s), age %u, profile %s\n",
               static_cast<long>(c.year), c.month, c.day, c.hour, c.minute, c.second,
               static_cast<unsigned long>(startUtc),
               p.maxHr, orNone(Text::maxSourceLabel(p.maxSource)),
               p.restHr, orNone(Text::restSourceLabel(p.restSource)),
               p.ageYears, profileName(p.status));
}

size_t headerLine(char* buf, size_t cap)
{
    return put(buf, cap,
               "window,active_s,good_s,outcome,speed_m_min,spread_pct,grade_pct,hr_avg,hr_min,hr_max,cost,scale,lag\n");
}

size_t windowLine(const WindowLog& w, char* buf, size_t cap)
{
    return put(buf, cap, "%u,%lu,%u,%s,%s,%s,%s,%s,%u,%u,%s,%s,%u\n",
               w.index, static_cast<unsigned long>(w.activeSec), w.goodSec, outcomeName(w.outcome),
               X10(w.speedMMinX10).s, X10(w.spreadX10).s, X10(w.gradeX10).s, X10(w.hrX10).s,
               w.hrMin, w.hrMax, X10(w.costX10).s, X1000(w.speedScaleX1000).s, w.afterChange ? 1u : 0u);
}

size_t resultLine(const RunResult& r, const Profile& p, uint8_t runMaxHr, char* buf, size_t cap)
{
    return put(buf, cap,
               "# result %s (profile %s), vo2max %s, windows %u, too_easy %u, above_max %u, "
               "implausible %u, max_hr %u (%s), rest_hr %u (%s), run_max_hr %u\n",
               statusName(r.status), profileName(r.profileStatus), X10(r.vo2x10).s, r.windowsUsed,
               r.tooEasy, r.aboveMax, r.implausible,
               p.maxHr, orNone(Text::maxSourceLabel(p.maxSource)),
               p.restHr, orNone(Text::restSourceLabel(p.restSource)), runMaxHr);
}

size_t whatIfLine(const RunResult& r, uint16_t shownX10, uint16_t shownWithinX10, char* buf, size_t cap)
{
    // 0.0 is "too few windows" for the per-run figures, "no runs" for the shown ones.
    return put(buf, cap,
               "# what-if (not used yet) after_change %u, without_lag %s, first_half %s, second_half %s, "
               "spread %s, shown %s, shown_%ud %s\n",
               r.afterChange, X10(r.withoutLagX10).s, X10(r.firstHalfX10).s, X10(r.secondHalfX10).s,
               X10(r.spreadX10).s, X10(shownX10).s, static_cast<unsigned>(Config::kMaxRunAgeDays),
               X10(shownWithinX10).s);
}

}  // namespace RunVo2::Log
