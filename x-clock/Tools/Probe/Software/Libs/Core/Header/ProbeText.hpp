/**
 ******************************************************************************
 * @file    ProbeText.hpp
 * @brief   Every line the probe writes or shows, made here so the host tests
 *          can check each one: the CSV rows and the scan dump (for the log
 *          files), and the seven rows, the ring and the trend (for the screen).
 *
 * Integers only, no %f (the standing embedded rules), and the 64-bit values
 * are printed as 32-bit halves where they can occur. Every writer fills the
 * caller's buffer, never past it, and returns the length written. A screen row
 * is never longer than kRowChars - 1 characters.
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_PROBE_TEXT_HPP
#define CLOCK_PROBE_PROBE_TEXT_HPP

#include <cstddef>
#include <cstdint>

#include "Fixed.hpp"
#include "ProbeView.hpp"
#include "Scan.hpp"
#include "WeekView.hpp"

namespace ClockProbe
{

/// A figure the watch has not given: left empty in a CSV row.
inline constexpr int32_t kUnknown = INT32_MIN;

// ---- the log files --------------------------------------------------------------------------

/// One row of min_<yyyymmdd>.csv.
struct MinuteRow {
    uint32_t utc          = 0;
    uint8_t  hour         = 0;
    uint8_t  minute       = 0;
    bool     wristOn      = false;
    int32_t  battX100     = kUnknown;   ///< percent x 100
    int32_t  mv           = kUnknown;
    int32_t  ma           = kUnknown;   ///< milliamps x 100 (the average drain is about 0.55 mA); negative is discharge
    int32_t  avgMa        = kUnknown;   ///< milliamps x 100
    int32_t  mah          = kUnknown;   ///< remaining capacity, mAh x 10
    int8_t   charging     = -1;         ///< -1 unknown
    int32_t  steps        = kUnknown;   ///< today
    int32_t  active       = kUnknown;   ///< active minutes today
    int32_t  floors       = kUnknown;
    uint32_t wristToday   = 0;
    uint32_t gestureToday = 0;
    uint16_t reports      = 0;          ///< GUI reports that arrived this minute (12 if it ticked all of it)
    int32_t  fpsX10       = kUnknown;
    uint32_t maxGapMs     = 0;
    uint16_t resumes      = 0;
    uint16_t suspends     = 0;
    uint32_t starts       = 0;
};

const char* minuteHeader();
size_t      formatMinuteRow(char* out, size_t cap, const MinuteRow& r);

/// days.csv: the watch's own day figures, once an hour. The history the ring would need.
const char* daysHeader();
size_t      formatDaysRow(char* out, size_t cap, uint32_t yyyymmdd, uint8_t hour, int32_t steps, int32_t active,
                          int32_t floors, bool wristOn);

/// events.csv: "utc,uptime_s,event,detail". A comma or line break in @p detail becomes a semicolon or space.
const char* eventsHeader();
size_t      formatEventLine(char* out, size_t cap, uint32_t utc, uint32_t uptimeS, const char* what,
                            const char* detail);

/// The human-readable scan dump (scan_<yyyymmdd>.txt), written in pieces as the scan goes.
size_t formatScanHeader(char* out, size_t cap, const char* localText, const char* why);
size_t formatListHeader(char* out, size_t cap, const char* dir, const ListResult& r);
size_t formatListEntry(char* out, size_t cap, const ListEntry& e);
size_t formatFileLine(char* out, size_t cap, const Known& k, const FileResult& r, Verdict v);
/// The file's content, indented and ended with a newline (cut at @p maxBytes).
size_t formatFileContent(char* out, size_t cap, const char* text, size_t maxBytes);

// ---- the screen -----------------------------------------------------------------------------

void rowFiles(ViewRow& row, const ScanData& scan);
void rowList(ViewRow& row, const ScanData& scan);
/// @p state: -1 not tried yet, 0 failed, 1 worked.
void rowLog(ViewRow& row, int8_t state);
void rowWrist(ViewRow& row, bool modeOn, uint32_t today, bool any, uint32_t ageMs);
void rowFrames(ViewRow& row, bool haveRate, uint16_t fpsX10, uint16_t resumes);
/// @p todayYmd is yyyymmdd, 0 when the clock is unset.
void rowPlan(ViewRow& row, const ScanData& scan, uint32_t todayYmd);
void rowVo2(ViewRow& row, const ScanData& scan);

/// The ring's seven grades and today's index.
void ringFromWeek(ViewData& view, const WeekView& week);
/// The VO2max trend as heights 2..20 (a flat trend is mid-height).
void trendFromVo2(ViewData& view, const ScanData& scan);

/// "4s", "12m", "3h", "2d": the age of an event, short.
size_t formatAge(char* out, size_t cap, uint32_t ms);

} // namespace ClockProbe

#endif // CLOCK_PROBE_PROBE_TEXT_HPP
