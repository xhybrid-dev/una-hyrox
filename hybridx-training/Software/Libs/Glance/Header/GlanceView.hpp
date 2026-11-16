/**
 ******************************************************************************
 * @file    GlanceView.hpp
 * @brief   What X Training's glance has to say, from the app's shared file and the
 *          watch's date: pure, host-tested (GlanceViewTest).
 *
 * The app writes ../SharedData/HybridX/training_today.json (TodayFile.hpp). The
 * glance reads it, picks what applies today (TodayFile::pick: a session, done, a
 * rest day, paused, before the start, complete, or "open the app" for a file that
 * is not for today) and works out the week's seven pips. Words and shapes are
 * GlanceLayout's. A file that fails any check reads as "no file", never a crash.
 ******************************************************************************
 */

#ifndef TRAINING_GLANCE_VIEW_HPP
#define TRAINING_GLANCE_VIEW_HPP

#include <cstddef>
#include <cstdint>

#include "Date.hpp"
#include "PlanTypes.hpp"
#include "TodayFile.hpp"

namespace TrainingGlance
{

/// How a day of this week shows in the row of seven pips.
enum class Pip : uint8_t {
    None = 0,   ///< Not a day of the plan.
    Rest,       ///< No session that day.
    ToCome,     ///< A session still to come.
    Today,      ///< Today's session, not yet done.
    Done,       ///< Done or part done.
    Missed,     ///< Missed or skipped, or passed with nothing logged.
};

struct View {
    XTraining::TodayFile::State state = XTraining::TodayFile::State::NoFile;

    // Today's row, for a session, done, skipped or a rest day.
    XTraining::Kind kind = XTraining::Kind::Rest;
    char what[XTraining::TodayDay::kWhatLen] = {};
    char pace[XTraining::TodayDay::kPaceLen] = {};

    // The next session after today in the file.
    bool            hasNext      = false;
    bool            nextTomorrow = false;
    uint8_t         nextWeekday  = 0;   ///< 0 Monday .. 6 Sunday.
    XTraining::Kind nextKind     = XTraining::Kind::Rest;
    char            nextWhat[XTraining::TodayDay::kWhatLen] = {};

    XTraining::Ymd startDate = 0;       ///< The plan's first day, for "Plan starts".

    // The week's pips and the plan's progress; shown only when hasWeek.
    bool    hasWeek = false;
    Pip     pip[7]  = {};
    uint8_t weekNo  = 0;                ///< Today's run week, 1-based.
    uint8_t weeks   = 0;
};

/// The shared file's path, from the glance's own folder.
constexpr const char* kSharedFile = XTraining::TodayFile::kPath;

/// Largest file the glance reads.
constexpr size_t kMaxFileBytes = XTraining::TodayFile::kMaxJsonBytes;

/// The view for @p today (yyyymmdd; 0 = the watch has no date) from @p data (null = no file).
void makeView(const XTraining::TodayData* data, XTraining::Ymd today, View& out);

/// As makeView, from the file's text; text that does not read is "no file".
void makeView(const char* text, size_t len, XTraining::Ymd today, View& out);

}  // namespace TrainingGlance

#endif  // TRAINING_GLANCE_VIEW_HPP
