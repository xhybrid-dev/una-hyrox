/**
 ******************************************************************************
 * @file    DashWords.cpp
 * @brief   The words of the family dashboard (see the header).
 ******************************************************************************
 */

#include "gui/copy/DashWords.hpp"

#include <cstdio>
#include <cstring>

#include "PlanText.hpp"
#include "PlanTypes.hpp"
#include "LoadFormText.hpp"
#include "TodayFile.hpp"
#include "WeekMath.hpp"
#include "WeighText.hpp"
#include "WeightUnit.hpp"
#include "gui/copy/Coach.hpp"
#include "gui/copy/WheelFontWidths.hpp"

namespace DashWords
{

namespace
{
/// U+00B7 MIDDLE DOT, which every text face carries.
constexpr const char* kDot = "\xC2\xB7";

unsigned u(uint32_t v) { return static_cast<unsigned>(v); }

using State = XTraining::TodayFile::State;

/// The first of @p choices whose shown length fits, else the last (the shortest).
void pickFirst(char* out, size_t size, const char* const* choices, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        if (choices[i] != nullptr && choices[i][0] != '\0' && tipPx(choices[i]) <= kMaxTipPx) {
            snprintf(out, size, "%s", choices[i]);
            return;
        }
    }
    snprintf(out, size, "%s", n > 0 && choices[n - 1] != nullptr ? choices[n - 1] : "");
}

void setRow(Row& r, const char* label, const char* tip, uint8_t tone = 0)
{
    snprintf(r.label, sizeof(r.label), "%s", label);
    snprintf(r.tip, sizeof(r.tip), "%s", tip);
    r.tone = tone;
}

/// X Training's name for a kind of session, as its glance shows it ("Easy" for easy with strides).
const char* planName(uint8_t kind)
{
    if (kind >= XTraining::kKindCount) {
        return "Session";
    }
    const auto k = static_cast<XTraining::Kind>(kind);
    return k == XTraining::Kind::EasyStrides ? "Easy" : XTraining::PlanText::shortName(k);
}

const char* monthShort(int m)
{
    static const char* const kMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    return (m >= 1 && m <= 12) ? kMonths[m - 1] : "";
}

const char* weekdayShortMon(uint8_t mon0)
{
    static const char* const kDays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    return kDays[mon0 % 7];
}
} // namespace

namespace
{
/// The width of @p s in @p table (1/16 px, ASCII then the middle dot), rounded up to whole px.
uint16_t widthPx(const char* s, const uint16_t (&table)[96])
{
    uint16_t widest = 0;
    for (uint16_t w : table) {
        widest = w > widest ? w : widest;
    }
    uint32_t sum = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(s); p != nullptr && *p != 0; ++p) {
        if (*p >= 0x20u && *p < 0x7Fu) {
            sum += table[*p - 0x20u];
        } else if (*p == 0xC2u && p[1] == 0xB7u) {   // U+00B7 MIDDLE DOT
            sum += table[95];
            ++p;
        } else if ((*p & 0xC0u) != 0x80u) {          // anything else: the widest, once per character
            sum += widest;
        }
    }
    return static_cast<uint16_t>((sum + 15u) / 16u);
}
} // namespace

uint16_t tipPx(const char* s)
{
    return widthPx(s, WheelFontWidths::kItalic18);
}

uint16_t labelPx(const char* s)
{
    return widthPx(s, WheelFontWidths::kSemiBold25);
}

uint16_t legendPx(const char* s)
{
    return widthPx(s, WheelFontWidths::kRegular16);
}

uint16_t smallPx(const char* s)
{
    return widthPx(s, WheelFontWidths::kRegular14);
}

// -- The training load graphs --------------------------------------------------------------

void dayMonth(int32_t day, char* out, size_t size)
{
    const Streak::WeekMath::Civil c = Streak::WeekMath::civilFromDays(day);
    snprintf(out, size, "%u %s", u(c.day), monthShort(static_cast<int>(c.month)));
}

void graphSpan(int32_t first, int32_t last, char* out, size_t size)
{
    char a[12], b[12];
    dayMonth(first, a, sizeof(a));
    dayMonth(last, b, sizeof(b));
    snprintf(out, size, "%s - %s", a, b);
}

void scaleLabel(int32_t value, int32_t unit, bool sign, char* out, size_t size)
{
    const int32_t un    = unit < 1 ? 1 : unit;
    const int32_t whole = value >= 0 ? (value + un / 2) / un : -((-value + un / 2) / un);
    if (sign && whole > 0) {
        snprintf(out, size, "+%ld", static_cast<long>(whole));
    } else {
        snprintf(out, size, "%ld", static_cast<long>(whole));
    }
}

void fitnessLegend(uint16_t fitness, char* out, size_t size)
{
    snprintf(out, size, "Fitness %u", u(fitness));
}

void fatigueLegend(uint16_t fatigue, char* out, size_t size)
{
    snprintf(out, size, "Fatigue %u", u(fatigue));
}

void formLegend(int16_t form, uint8_t formWord, char* out, size_t size)
{
    char n[8];
    Streak::formatFormNumber(n, sizeof(n), form);
    const auto word  = static_cast<Streak::FormWord>(formWord);
    const bool known = word != Streak::FormWord::Learning && word != Streak::FormWord::NoLoad;
    char full[48];
    snprintf(full, sizeof(full), "Form %s, %s", n, Streak::formWordName(word));
    if (known && legendPx(full) <= kMaxLegendPx) {
        snprintf(out, size, "%s", full);
    } else {
        snprintf(out, size, "Form %s", n);
    }
}

void weekLoadLegend(uint32_t weekLoad, char* out, size_t size)
{
    snprintf(out, size, "This week %lu", static_cast<unsigned long>(weekLoad > 99999u ? 99999u : weekLoad));
}

void todayLoadLegend(uint16_t todayLoad, char* out, size_t size)
{
    snprintf(out, size, "Today %u", u(todayLoad));
}

void duration(uint32_t minutes, char* out, size_t size)
{
    if (minutes < 60u) {
        snprintf(out, size, "%u min", u(minutes));
    } else {
        snprintf(out, size, "%u h %02u", u(minutes / 60u), u(minutes % 60u));
    }
}

void distance(uint32_t metres, char* out, size_t size)
{
    if (metres >= 99950u) {   // 100 km and up: whole kilometres
        snprintf(out, size, "%u km", u((metres + 500u) / 1000u));
        return;
    }
    const uint32_t tenths = (metres + 50u) / 100u;   // to the nearest 100 m
    snprintf(out, size, "%u.%u km", u(tenths / 10u), u(tenths % 10u));
}

const char* sportNoun(Streak::Kind kind, uint32_t count)
{
    const bool one = count == 1;
    switch (kind) {
        case Streak::Kind::Run:     return one ? "run" : "runs";
        case Streak::Kind::Ride:    return one ? "ride" : "rides";
        case Streak::Kind::Walk:    return one ? "walk" : "walks";
        case Streak::Kind::Workout: return one ? "workout" : "workouts";
        case Streak::Kind::Row:     return one ? "row" : "rows";
        default:                    return one ? "session" : "sessions";
    }
}

void sportTip(Streak::Kind kind, const Streak::SportSum& s, char* out, size_t size)
{
    char t[16], d[16];
    duration(s.minutes, t, sizeof(t));
    distance(s.metres, d, sizeof(d));
    // "2 runs · 1 h 05 · 14.2 km" is 213 px: with a distance the count usually stands alone,
    // as the screen's first row ("5 sessions · 3 h 20") has already said what it counts.
    char full[64], mid[48], timed[40], small[24];
    const char* noun = sportNoun(kind, s.sessions);
    snprintf(timed, sizeof(timed), "%u %s %s %s", u(s.sessions), noun, kDot, t);
    snprintf(small, sizeof(small), "%u %s", u(s.sessions), noun);
    if (s.metres > 0) {
        snprintf(full, sizeof(full), "%u %s %s %s %s %s", u(s.sessions), noun, kDot, t, kDot, d);
        snprintf(mid, sizeof(mid), "%u %s %s %s %s", u(s.sessions), kDot, t, kDot, d);
    } else {
        snprintf(full, sizeof(full), "%s", timed);
        snprintf(mid, sizeof(mid), "%u %s %s", u(s.sessions), kDot, t);
    }
    const char* const choices[] = {full, mid, timed, small};
    pickFirst(out, size, choices, 4);
}

void allTip(const Streak::SportSum& s, char* out, size_t size)
{
    char t[16], full[48], small[32];
    duration(s.minutes, t, sizeof(t));
    snprintf(full, sizeof(full), "%u session%s %s %s", u(s.sessions), s.sessions == 1 ? "" : "s", kDot, t);
    snprintf(small, sizeof(small), "%u %s %s", u(s.sessions), kDot, t);
    const char* const choices[] = {full, small};
    pickFirst(out, size, choices, 2);
}

void menuTip(const Streak::WeekSum& week, uint8_t manual, char* out, size_t size)
{
    if (week.all.sessions == 0) {
        if (manual > 0) {
            snprintf(out, size, "%u by hand", u(manual));
        } else {
            snprintf(out, size, "Nothing yet");
        }
        return;
    }
    char t[16], d[16];
    duration(week.all.minutes, t, sizeof(t));
    if (week.all.metres == 0) {
        snprintf(out, size, "%s", t);
        return;
    }
    distance(week.all.metres, d, sizeof(d));
    snprintf(out, size, "%s %s %s", t, kDot, d);
}

void manualTip(uint8_t count, char* out, size_t size)
{
    snprintf(out, size, "%u session%s", u(count), count == 1 ? "" : "s");
}

// -- Your numbers -------------------------------------------------------------------

bool planRow(const Streak::PlanToday& p, Row& out)
{
    const auto state = static_cast<State>(p.state);
    const char* name = planName(p.kind);
    char        a[48], b[48];
    switch (state) {
        case State::Session: {
            snprintf(a, sizeof(a), "%s %s", name, p.what);
            const char* const choices[] = {a, name};
            setRow(out, "Today's plan", "");
            pickFirst(out.tip, sizeof(out.tip), choices, 2);
            return true;
        }
        case State::Done: {
            snprintf(a, sizeof(a), "Done: %s", name);
            const char* const choices[] = {a, "Done"};
            setRow(out, "Today's plan", "", 1);
            pickFirst(out.tip, sizeof(out.tip), choices, 2);
            return true;
        }
        case State::Skipped: {
            snprintf(a, sizeof(a), "Skipped: %s", name);
            const char* const choices[] = {a, "Skipped"};
            setRow(out, "Today's plan", "");
            pickFirst(out.tip, sizeof(out.tip), choices, 2);
            return true;
        }
        case State::Rest:
            setRow(out, "Today's plan", "Rest day");
            return true;
        case State::Paused:
            setRow(out, "Today's plan", "Plan paused");
            return true;
        case State::BeforeStart: {
            const int m = XTraining::Date::month(p.startDate);
            const int d = XTraining::Date::day(p.startDate);
            snprintf(b, sizeof(b), "Starts %d %s", d, monthShort(m));
            setRow(out, "Today's plan", p.startDate != 0 ? b : "Not started");
            return true;
        }
        case State::Complete:
            setRow(out, "Today's plan", "Plan complete", 1);
            return true;
        case State::Stale:
            setRow(out, "Today's plan", "Open X Training", 2);
            return true;
        case State::NoPlan:
            setRow(out, "Today's plan", "No plan yet");
            return true;
        case State::ClockUnset:
            setRow(out, "Today's plan", "Date not set", 2);
            return true;
        case State::NoFile:
            break;
    }
    return false;
}

bool paceRow(const Streak::PlanToday& p, Row& out)
{
    if (static_cast<State>(p.state) != State::Session || p.pace[0] == '\0') {
        return false;
    }
    setRow(out, "Pace", p.pace);
    return true;
}

bool nextRow(const Streak::PlanToday& p, Row& out)
{
    const auto state = static_cast<State>(p.state);
    if (p.hasNext == 0 || (state != State::Done && state != State::Skipped && state != State::Rest)) {
        return false;
    }
    const char* day  = p.nextTomorrow ? "Tomorrow" : weekdayShortMon(p.nextWeekday);
    const char* name = planName(p.nextKind);
    char        a[48], b[32];
    snprintf(a, sizeof(a), "%s: %s %s", day, name, p.nextWhat);
    snprintf(b, sizeof(b), "%s: %s", day, name);
    const char* const choices[] = {p.nextWhat[0] != '\0' ? a : b, b, name};
    setRow(out, "Next", "");
    pickFirst(out.tip, sizeof(out.tip), choices, 3);
    return true;
}

bool vo2Row(const Streak::FamilyView& v, Row& out)
{
    if ((v.flags & Streak::FamilyView::kVo2) == 0 || v.vo2.x10 == 0) {
        return false;
    }
    char a[32];
    snprintf(a, sizeof(a), "%u.%u %s %u run%s", u(v.vo2.x10 / 10u), u(v.vo2.x10 % 10u), kDot, u(v.vo2.runs),
             v.vo2.runs == 1 ? "" : "s");
    setRow(out, "VO2max", a);
    return true;
}

bool weightRow(const Streak::FamilyView& v, Row& out)
{
    if ((v.flags & Streak::FamilyView::kWeight) == 0) {
        return false;
    }
    char a[32];
    XWeighIn::formatWeightWithUnit(v.weight.trend, XWeighIn::weightUnitFromConfig(v.weight.unit), a, sizeof(a));
    setRow(out, "Weight", a);
    return true;
}

bool weightChangeRow(const Streak::FamilyView& v, Row& out)
{
    if ((v.flags & Streak::FamilyView::kWeight) == 0) {
        return false;
    }
    const int32_t ago = v.today - v.weight.lastDay;
    char          a[32];
    if (v.clockOk != 0 && ago > 7) {
        snprintf(a, sizeof(a), "%ld days ago", static_cast<long>(ago));
        setRow(out, "Last weigh-in", a, 2);
        return true;
    }
    if (v.weight.hasWeekChange == 0) {
        return false;   // under a week of weigh-ins: X Weigh-in shows "--", so nothing here
    }
    const XWeighIn::WeightUnit unit = XWeighIn::weightUnitFromConfig(v.weight.unit);
    char change[16];
    XWeighIn::formatWeightChange(v.weight.weekChange, unit, 1, change, sizeof(change));
    snprintf(a, sizeof(a), "%s %s this week", change, XWeighIn::changeUnitName(unit));
    setRow(out, "Trend", a);
    return true;
}

void numbersMenuTip(const Streak::FamilyView& v, char* out, size_t size)
{
    Row        r;
    const auto state = static_cast<State>(v.plan.state);
    const bool planSays = (v.flags & Streak::FamilyView::kPlan) != 0 && state != State::NoPlan
                          && state != State::ClockUnset && planRow(v.plan, r);
    if (planSays) {
        snprintf(out, size, "%s", r.tip);
    } else if (vo2Row(v, r)) {
        snprintf(out, size, "VO2max %u.%u", u(v.vo2.x10 / 10u), u(v.vo2.x10 % 10u));
    } else if (weightRow(v, r)) {
        snprintf(out, size, "%s", r.tip);
    } else {
        snprintf(out, size, "From the X apps");
    }
}

bool bankedPlanLine(const Streak::FamilyView& v, char* out, size_t size)
{
    if ((v.flags & Streak::FamilyView::kPlan) == 0 || static_cast<State>(v.plan.state) != State::Session) {
        return false;
    }
    const char* name = planName(v.plan.kind);
    char        a[48];
    snprintf(a, sizeof(a), "Banked. %s today", name);
    if (std::strlen(a) > Coach::kMaxCoachChars) {   // the coach's line is held to characters
        snprintf(a, sizeof(a), "%s today", name);
    }
    snprintf(out, size, "%s", a);
    return true;
}

uint8_t numbersRows(const Streak::FamilyView& v, Row* rows)
{
    // One row per app at least, so the athlete sees where each figure comes from (and the
    // wheel never shows one row twice, as it wraps a short list round).
    uint8_t n = 0;
    Row     row;
    auto    take = [&](bool ok) {
        if (ok && n < kMaxRows) {
            rows[n++] = row;
        }
        row = Row {};
    };
    if ((v.flags & Streak::FamilyView::kPlan) != 0 && planRow(v.plan, row)) {
        take(true);
        take(paceRow(v.plan, row));
        take(nextRow(v.plan, row));
    } else {
        setRow(row, "Today's plan", "Needs X Training");
        take(true);
    }
    if (!vo2Row(v, row)) {
        setRow(row, "VO2max", "Needs X Run");
    }
    take(true);
    if (weightRow(v, row)) {
        take(true);
        take(weightChangeRow(v, row));
    } else {
        setRow(row, "Weight", "Needs X Weigh-in");
        take(true);
    }
    return n;
}

} // namespace DashWords
