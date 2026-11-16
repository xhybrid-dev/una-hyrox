/**
 ******************************************************************************
 * @file    TodayFile.hpp
 * @brief   training_today.json: what the glance reads (TP6, brief section 12).
 *
 * The app writes ../SharedData/HybridX/training_today.json whenever the plan
 * changes or the app opens; the glance (a separate .uapp, display only) reads it
 * and never writes. One file holds the plan's days from the Monday of the week
 * it was written through 14 days after, so the glance stays right for a fortnight
 * without the app being opened.
 *
 *   {"v":1,"plan":1,"wks":10,"ps":0,"s":20261012,"e":20261220,
 *    "f":20261123,"t":20261207,
 *    "d":[[20261123,7,1,6,"4 x 4 min","4:18/km"],[20261124,0,0,6,"",""],...]}
 *
 *   plan  the PlanId; 0 = no plan (written when a plan ends). Then nothing else is read.
 *   wks   the plan's weeks, for the progress bar.
 *   ps    the day the plan was paused from, 0 = not paused.
 *   s, e  the plan's first and last day.
 *   f, t  the window the file covers (first and last day); the glance is right
 *         for any day in it, and says "open the app" for any other.
 *   d     the plan's days in the window, by date: [date, kind, status, runWeek
 *         (1-based), "what", "pace"]. Days outside the plan have no row. What
 *         the glance shows of a session is pre-worded by the app (PlanText::
 *         glanceWhat, the pace in the runner's units), because the glance has
 *         neither the plan's data nor the settings.
 *
 * Integers only, none negative (the family's rule); the strings are printable
 * ASCII with no quote or backslash (setText() makes them so). Every value is
 * range-checked on reading; a file that fails any check reads as no data, never
 * a crash. Fixed-size, no heap: the glance's service has a 10 KB stack.
 ******************************************************************************
 */

#ifndef XTRAINING_TODAY_FILE_HPP
#define XTRAINING_TODAY_FILE_HPP

#include "Date.hpp"
#include "PlanTypes.hpp"

#include <cstddef>
#include <cstdint>

namespace XTraining {

/// How a planned day stands, the app's DayStatus with "to come" as 0.
enum class TodayStatus : uint8_t { ToCome = 0, Done = 1, PartDone = 2, Missed = 3, Skipped = 4 };

struct TodayDay {
    static constexpr size_t kWhatLen = 20;   ///< "100 min, 20 fast" is 16.
    static constexpr size_t kPaceLen = 12;   ///< "12:34/mi" is 8.

    Ymd         date    = 0;
    Kind        kind    = Kind::Rest;
    TodayStatus status  = TodayStatus::ToCome;
    uint8_t     runWeek = 0;                 ///< 1-based.
    char        what[kWhatLen] = {};
    char        pace[kPaceLen] = {};
};

struct TodayData {
    static constexpr uint8_t kMaxDays = 21;  ///< The week's Monday through 14 days after a Sunday.
    static constexpr uint8_t kMaxWeeks = 40;

    uint8_t  plan   = 0;       ///< The PlanId; 0 = no plan.
    uint8_t  weeks  = 0;
    Ymd      pausedFrom = 0;
    Ymd      start  = 0;
    Ymd      end    = 0;
    Ymd      first  = 0;
    Ymd      last   = 0;
    uint8_t  count  = 0;
    TodayDay day[kMaxDays];

    /// The row for @p date, or null (a day outside the plan, or the window).
    const TodayDay* find(Ymd date) const;
};

namespace TodayFile {

/// The file, from the app's folder and from the glance's (both reach the shared folder).
constexpr const char* kPath = "../SharedData/HybridX/training_today.json";

/// The largest file read or written: 21 rows of about 50 bytes and a header.
constexpr size_t kMaxJsonBytes = 1536;

/// The glance is right for @p today only inside the window.
constexpr int32_t kWindowDays = 14;

/// Copy @p src into @p dst (cap bytes), keeping printable ASCII and dropping anything
/// else, and quotes and backslashes in particular. Always terminated.
void setText(char* dst, size_t cap, const char* src);

/// Write the file into @p buf. The length, or 0 if it did not fit or @p data is unusable.
size_t write(const TodayData& data, char* buf, size_t cap);

/// Read the file. False (and @p out cleared) for anything that fails a check.
bool read(const char* text, size_t len, TodayData& out);

/// What the glance has to say, decided once for the app's check and the glance.
enum class State : uint8_t {
    NoFile,       ///< Never written: the app has not been opened since the glance.
    NoPlan,       ///< The file says there is no plan.
    ClockUnset,   ///< The watch has no date.
    Stale,        ///< Today is outside the window: the app has not been opened for a fortnight.
    Paused,
    BeforeStart,
    Complete,
    Session,      ///< A session today, not yet done.
    Done,         ///< Today's session is done or part done.
    Skipped,      ///< Today's session was skipped.
    Rest,         ///< A rest day, or a day of the plan with nothing in it.
};

struct Pick {
    State           state = State::NoFile;
    const TodayDay* today = nullptr;   ///< Today's row, for Session, Done, Skipped and Rest.
    const TodayDay* next  = nullptr;   ///< The next session after today in the file, or null.
};

/// @p data is null when there is no usable file; @p today is yyyymmdd, 0 = no date.
Pick pick(const TodayData* data, Ymd today);

}  // namespace TodayFile
}  // namespace XTraining

#endif  // XTRAINING_TODAY_FILE_HPP
