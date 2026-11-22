/**
 ******************************************************************************
 * @file    DashWords.hpp
 * @brief   The words of the family dashboard (NOTES "S7"): the By sport and
 *          Your numbers screens, and their menu rows.
 *
 * Pure C++ with fixed buffers and integer formatting, as the Coach, so the host
 * tests check every line and its width (DashTextTest). A row is a label (the
 * wheel's big line, SemiBold 25) and a tip (its italic line under it, Italic
 * 18). Widths are measured in pixels with the fonts' own advances
 * (WheelFontWidths.hpp): a tip wider than kMaxTipPx falls back to a shorter
 * wording, because the wheel wraps a long line rather than cutting it.
 *
 * X Training's sessions are named as X Training names them (PlanText::
 * shortName) and weights shown as X Weigh-in shows them (WeighText), from the
 * copies in Libs/Family, so the numbers read the same in every app.
 ******************************************************************************
 */

#ifndef STREAK_DASH_WORDS_HPP
#define STREAK_DASH_WORDS_HPP

#include <cstddef>
#include <cstdint>

#include "DashView.hpp"
#include "StreakTypes.hpp"

namespace DashWords
{

/// The widest tip, px. A selected row's tip has about 190 px between the bezel arcs, but the
/// row under it sits lower on the disc: there Training load's "Learning: day 1 of 14" (189 px)
/// loses its first letter, while "2 · 1 h 06 · 11.2 km" (168 px) shows whole (NOTES "S7").
constexpr uint16_t kMaxTipPx = 172;
/// The widest label, px: "Your numbers" is 180 in SemiBold 25, "Training load" 168.
constexpr uint16_t kMaxLabelPx = 182;
/// The widest screen title, px: the SDK's Title is a 120 px label (Title.cpp) in Italic 18,
/// the tips' face. "Your numbers" (127) does not fit, so that screen's title is "Numbers".
constexpr uint16_t kMaxTitlePx = 120;
constexpr const char* kSportTitle   = "By sport";
constexpr const char* kNumbersTitle = "Numbers";

/// Text widths in whole px, rounded up: a tip (Italic 18) and a label (SemiBold 25). A
/// character outside the fonts counts as their widest.
uint16_t tipPx(const char* s);
uint16_t labelPx(const char* s);

/// "45 min", "1 h 05", "12 h 30".
void duration(uint32_t minutes, char* out, size_t size);
/// "14.2 km", "0.4 km", "123 km" (whole kilometres from 100 km).
void distance(uint32_t metres, char* out, size_t size);

/// "run" or "runs", ... and "session" or "sessions" for the kinds without a plural.
const char* sportNoun(Streak::Kind kind, uint32_t count);

/// One kind's tip: "2 runs · 1 h 05 · 14.2 km", or a shorter form if that is too long.
void sportTip(Streak::Kind kind, const Streak::SportSum& s, char* out, size_t size);
/// Every kind's: "5 sessions · 3 h 20".
void allTip(const Streak::SportSum& s, char* out, size_t size);
/// The menu row's tip: "3 h 20 · 24.5 km", "3 h 20", "2 by hand" (manual logs only), or
/// "Nothing yet".
void menuTip(const Streak::WeekSum& week, uint8_t manual, char* out, size_t size);
/// The manual logs' tip: "1 session".
void manualTip(uint8_t count, char* out, size_t size);

// -- Your numbers -------------------------------------------------------------------

/// A row of Your numbers. tone: 0 plain, 1 done (lime), 2 needs a look (amber).
struct Row {
    char    label[24] {};
    char    tip[48] {};
    uint8_t tone = 0;
};

/// X Training's row for today: "Tempo 30 min", "Done: Tempo", "Rest day", "No plan yet", ...
/// False only with no file.
bool planRow(const Streak::PlanToday& p, Row& out);
/// Today's pace, when there is a session today with one: "4:18/km".
bool paceRow(const Streak::PlanToday& p, Row& out);
/// The next session, after a session done, skipped or a rest day: "Tomorrow: Easy 40 min".
bool nextRow(const Streak::PlanToday& p, Row& out);
/// X Run's VO2max: "52.3 · 5 runs".
bool vo2Row(const Streak::FamilyView& v, Row& out);
/// X Weigh-in's trend: "78.4 kg".
bool weightRow(const Streak::FamilyView& v, Row& out);
/// The trend's week: "-0.4 kg this week"; or, when the last weigh-in is over a week old,
/// "Last weigh-in" and "12 days ago".
bool weightChangeRow(const Streak::FamilyView& v, Row& out);
/// The menu row's tip: today's plan, else the VO2max, else the weight, else "From the X apps".
void numbersMenuTip(const Streak::FamilyView& v, char* out, size_t size);

/// Home's coach line when the week is banked and X Training still has a session for today,
/// in place of "Week banked. Rest up.", which would contradict the plan: "Banked. Tempo today",
/// or "Intervals today" when that is too long for the coach's line (Coach::kMaxCoachChars).
/// False with no session to do today.
bool bankedPlanLine(const Streak::FamilyView& v, char* out, size_t size);

/// Every row Your numbers shows, in order, into @p rows (room for kMaxRows): X Training's,
/// X Run's and X Weigh-in's, each with at least one row ("Needs X Run" when its app has left
/// nothing). Returns how many, 3 to 5 (pace and next never show together).
constexpr uint8_t kMaxRows = 7;
uint8_t numbersRows(const Streak::FamilyView& v, Row* rows);

} // namespace DashWords

#endif // STREAK_DASH_WORDS_HPP
