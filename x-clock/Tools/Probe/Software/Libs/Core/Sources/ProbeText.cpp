/**
 ******************************************************************************
 * @file    ProbeText.cpp
 * @brief   See the header.
 ******************************************************************************
 */

#include "ProbeText.hpp"

#include <cstdio>
#include <cstring>

namespace ClockProbe
{

namespace
{

/// Appends to a caller's buffer and never passes its end; the buffer stays NUL-terminated.
class Out
{
public:
    Out(char* buf, size_t cap) : mBuf(buf), mCap(cap)
    {
        if (mCap > 0) {
            mBuf[0] = '\0';
        }
    }
    void ch(char c)
    {
        if (mCap > 0 && mLen + 1 < mCap) {
            mBuf[mLen++] = c;
            mBuf[mLen]   = '\0';
        }
    }
    void str(const char* s)
    {
        while (s != nullptr && *s != '\0') {
            ch(*s++);
        }
    }
    void u(uint32_t v)
    {
        char t[12];
        std::snprintf(t, sizeof(t), "%lu", static_cast<unsigned long>(v));
        str(t);
    }
    /// A signed figure; kUnknown leaves the field empty.
    void i(int32_t v)
    {
        if (v == kUnknown) {
            return;
        }
        char t[14];
        std::snprintf(t, sizeof(t), "%ld", static_cast<long>(v));
        str(t);
    }
    size_t len() const { return mLen; }

private:
    char*  mBuf;
    size_t mCap;
    size_t mLen = 0;
};

void setRow(ViewRow& row, Light light, const char* text)
{
    row.light = static_cast<uint8_t>(light);
    std::strncpy(row.text, text, kRowChars - 1);
    row.text[kRowChars - 1] = '\0';
}

const char* yn(bool b) { return b ? "1" : "0"; }

/// The longest plan line shown with its pace.
constexpr size_t kPlanChars = 20;

} // namespace

// ---------------------------------------------------------------------------------------------
// The log files
// ---------------------------------------------------------------------------------------------

const char* minuteHeader()
{
    return "utc,local,wrist_mode,batt_pct_x100,mv,ma_x100,avg_ma_x100,mah_x10,charging,steps,active_min,floors,"
           "wrist_today,gesture_today,gui_reports,gui_fps_x10,gui_max_gap_ms,resumes,suspends,starts\n";
}

size_t formatMinuteRow(char* out, size_t cap, const MinuteRow& r)
{
    Out o(out, cap);
    o.u(r.utc);
    o.ch(',');
    o.ch(static_cast<char>('0' + (r.hour / 10) % 10));
    o.ch(static_cast<char>('0' + r.hour % 10));
    o.ch(':');
    o.ch(static_cast<char>('0' + (r.minute / 10) % 10));
    o.ch(static_cast<char>('0' + r.minute % 10));
    o.ch(',');
    o.str(r.wristOn ? "on" : "off");
    o.ch(',');
    o.i(r.battX100);
    o.ch(',');
    o.i(r.mv);
    o.ch(',');
    o.i(r.ma);
    o.ch(',');
    o.i(r.avgMa);
    o.ch(',');
    o.i(r.mah);
    o.ch(',');
    if (r.charging >= 0) {
        o.u(static_cast<uint32_t>(r.charging));
    }
    o.ch(',');
    o.i(r.steps);
    o.ch(',');
    o.i(r.active);
    o.ch(',');
    o.i(r.floors);
    o.ch(',');
    o.u(r.wristToday);
    o.ch(',');
    o.u(r.gestureToday);
    o.ch(',');
    o.u(r.reports);
    o.ch(',');
    o.i(r.fpsX10);
    o.ch(',');
    o.u(r.maxGapMs);
    o.ch(',');
    o.u(r.resumes);
    o.ch(',');
    o.u(r.suspends);
    o.ch(',');
    o.u(r.starts);
    o.ch('\n');
    return o.len();
}

const char* daysHeader()
{
    return "date,hour,steps,active_min,floors,wrist_mode\n";
}

size_t formatDaysRow(char* out, size_t cap, uint32_t yyyymmdd, uint8_t hour, int32_t steps, int32_t active,
                     int32_t floors, bool wristOn)
{
    Out o(out, cap);
    o.u(yyyymmdd);
    o.ch(',');
    o.u(hour);
    o.ch(',');
    o.i(steps);
    o.ch(',');
    o.i(active);
    o.ch(',');
    o.i(floors);
    o.ch(',');
    o.str(wristOn ? "on" : "off");
    o.ch('\n');
    return o.len();
}

const char* eventsHeader()
{
    return "utc,uptime_s,event,detail\n";
}

size_t formatEventLine(char* out, size_t cap, uint32_t utc, uint32_t uptimeS, const char* what, const char* detail)
{
    Out o(out, cap);
    o.u(utc);
    o.ch(',');
    o.u(uptimeS);
    o.ch(',');
    o.str(what);
    o.ch(',');
    for (const char* p = detail; p != nullptr && *p != '\0'; ++p) {
        o.ch(*p == ',' ? ';' : ((*p == '\n' || *p == '\r') ? ' ' : *p));
    }
    o.ch('\n');
    return o.len();
}

size_t formatScanHeader(char* out, size_t cap, const char* localText, const char* why)
{
    Out o(out, cap);
    o.str("== scan ");
    o.str(localText);
    o.str(" (");
    o.str(why);
    o.str(") ==\n");
    return o.len();
}

size_t formatListHeader(char* out, size_t cap, const char* dir, const ListResult& r)
{
    Out o(out, cap);
    o.str("list ");
    o.str(dir);
    o.str(": exists=");
    o.str(yn(r.exists));
    o.str(" opened=");
    o.str(yn(r.opened));
    if (r.openedWithoutExist) {
        o.str(" OPEN-WITHOUT-EXIST");
    }
    o.str(" entries=");
    o.u(r.entries);
    o.str(" files=");
    o.u(r.files);
    o.str(" dirs=");
    o.u(r.dirs);
    if (r.full) {
        o.str(" (limit)");
    }
    o.ch('\n');
    return o.len();
}

size_t formatListEntry(char* out, size_t cap, const ListEntry& e)
{
    Out o(out, cap);
    o.str("  ");
    o.str(e.name);
    if (e.isDir) {
        o.str("/\n");
        return o.len();
    }
    o.str("  ");
    o.u(e.size);
    o.str(" B  utc ");
    o.u(e.utc);
    o.ch('\n');
    return o.len();
}

size_t formatFileLine(char* out, size_t cap, const Known& k, const FileResult& r, Verdict v)
{
    Out o(out, cap);
    o.str("file ");
    o.str(k.label);
    o.str(": ");
    if (!r.exists) {
        o.str("not there\n");
        return o.len();
    }
    o.str("exists");
    if (r.infoOk) {
        o.str(", size ");
        o.u(r.size);
        o.str(", utc ");
        o.u(r.utc);
    } else {
        o.str(", no info");
    }
    if (v != Verdict::SizeOnly) {
        o.str(r.readOk ? ", read ok" : (r.opened ? ", read failed" : ", cannot open"));
        if (r.truncated) {
            o.str(", truncated");
        }
    }
    o.str(", ");
    o.str(verdictName(v));
    o.ch('\n');
    return o.len();
}

size_t formatFileContent(char* out, size_t cap, const char* text, size_t maxBytes)
{
    Out o(out, cap);
    o.str("    ");
    size_t n = 0;
    for (const char* p = text; p != nullptr && *p != '\0' && n < maxBytes; ++p, ++n) {
        o.ch((*p == '\n' || *p == '\r') ? ' ' : *p);
    }
    o.ch('\n');
    return o.len();
}

// ---------------------------------------------------------------------------------------------
// The screen
// ---------------------------------------------------------------------------------------------

size_t formatAge(char* out, size_t cap, uint32_t ms)
{
    Out o(out, cap);
    const uint32_t s = ms / 1000u;
    if (s < 60u) {
        o.u(s);
        o.ch('s');
    } else if (s < 3600u) {
        o.u(s / 60u);
        o.ch('m');
    } else if (s < 86400u) {
        o.u(s / 3600u);
        o.ch('h');
    } else {
        o.u(s / 86400u);
        o.ch('d');
    }
    return o.len();
}

void rowFiles(ViewRow& row, const ScanData& scan)
{
    const uint8_t found = scan.found(), bad = scan.problems();
    char t[kRowChars + 12];
    if (bad > 0) {
        std::snprintf(t, sizeof(t), "Files %u of %u, %u bad", static_cast<unsigned>(found),
                      static_cast<unsigned>(kKnownCount), static_cast<unsigned>(bad));
    } else {
        std::snprintf(t, sizeof(t), "Files %u of %u", static_cast<unsigned>(found),
                      static_cast<unsigned>(kKnownCount));
    }
    setRow(row, found == 0 ? Light::Red : (bad > 0 ? Light::Amber : Light::Green), t);
}

void rowList(ViewRow& row, const ScanData& scan)
{
    const ListResult& s = scan.shared;
    const ListResult& h = scan.hybridx;
    char t[kRowChars + 12];
    if (s.opened && h.opened) {
        std::snprintf(t, sizeof(t), "List ok %u + %u", static_cast<unsigned>(s.entries),
                      static_cast<unsigned>(h.entries));
        setRow(row, Light::Green, t);
    } else if (s.opened) {
        std::snprintf(t, sizeof(t), "List ok %u, no HX", static_cast<unsigned>(s.entries));
        setRow(row, Light::Amber, t);
    } else if (s.exists || h.exists) {
        setRow(row, Light::Red, "List denied");
    } else if (s.openedWithoutExist || h.openedWithoutExist) {
        setRow(row, Light::Amber, "List: open w/o dir");
    } else {
        setRow(row, Light::Red, "List: no folder");
    }
}

void rowLog(ViewRow& row, int8_t state)
{
    if (state < 0) {
        setRow(row, Light::Grey, "Log write ?");
    } else if (state == 0) {
        setRow(row, Light::Red, "Log write FAILED");
    } else {
        setRow(row, Light::Green, "Log write ok");
    }
}

void rowWrist(ViewRow& row, bool modeOn, uint32_t today, bool any, uint32_t ageMs)
{
    char t[kRowChars + 12];
    if (!modeOn) {
        setRow(row, Light::Grey, "Wrist off today");
        return;
    }
    if (!any || ageMs == 0xFFFFFFFFu) {
        setRow(row, Light::Grey, "Wrist 0 so far");
        return;
    }
    char age[8];
    formatAge(age, sizeof(age), ageMs);
    std::snprintf(t, sizeof(t), "Wrist %lu, %s ago", static_cast<unsigned long>(today > 999 ? 999 : today), age);
    setRow(row, Light::Green, t);
}

void rowFrames(ViewRow& row, bool haveRate, uint16_t fpsX10, uint16_t resumes)
{
    if (!haveRate) {
        setRow(row, Light::Grey, "Ticks: waiting");
        return;
    }
    char t[kRowChars + 12];
    std::snprintf(t, sizeof(t), "Ticks %u.%u/s, R%u", static_cast<unsigned>(fpsX10 / 10u),
                  static_cast<unsigned>(fpsX10 % 10u), static_cast<unsigned>(resumes > 99 ? 99 : resumes));
    setRow(row, fpsX10 >= 90 ? Light::Green : Light::Amber, t);
}

void rowPlan(ViewRow& row, const ScanData& scan, uint32_t todayYmd)
{
    const ScanFile& f = scan.files[kIdxToday];
    if (!f.r.exists) {
        setRow(row, Light::Grey, "Plan: no file");
        return;
    }
    if (!scan.todayOk) {
        setRow(row, Light::Red, "Plan: bad file");
        return;
    }
    const XTraining::TodayFile::Pick pick = XTraining::TodayFile::pick(&scan.today, todayYmd);
    char t[64];
    switch (pick.state) {
        case XTraining::TodayFile::State::NoFile:      setRow(row, Light::Grey, "Plan: no file"); break;
        case XTraining::TodayFile::State::NoPlan:      setRow(row, Light::Grey, "Plan: none set"); break;
        case XTraining::TodayFile::State::ClockUnset:  setRow(row, Light::Grey, "Plan: no clock"); break;
        case XTraining::TodayFile::State::Stale:       setRow(row, Light::Amber, "Plan: open X Training"); break;
        case XTraining::TodayFile::State::Paused:      setRow(row, Light::Amber, "Plan: paused"); break;
        case XTraining::TodayFile::State::BeforeStart: setRow(row, Light::Grey, "Plan: not started"); break;
        case XTraining::TodayFile::State::Complete:    setRow(row, Light::Green, "Plan: complete"); break;
        case XTraining::TodayFile::State::Rest:        setRow(row, Light::Green, "Rest day"); break;
        case XTraining::TodayFile::State::Skipped:     setRow(row, Light::Grey, "Skipped today"); break;
        case XTraining::TodayFile::State::Session:
            if (pick.today != nullptr) {
                // The pace goes if the two together are over 20 characters (audit.py: wide glyphs reach the
                // ring beyond that): half a pace misleads, and the words are the point.
                const size_t both = std::strlen(pick.today->what) + 1 + std::strlen(pick.today->pace);
                if (both <= kPlanChars) {
                    std::snprintf(t, sizeof(t), "%s %s", pick.today->what, pick.today->pace);
                } else {
                    std::snprintf(t, sizeof(t), "%s", pick.today->what);
                }
                setRow(row, Light::Green, t);
            }
            break;
        case XTraining::TodayFile::State::Done:
            if (pick.today != nullptr) {
                std::snprintf(t, sizeof(t), "Done: %s", pick.today->what);
                setRow(row, Light::Green, t);
            }
            break;
    }
}

void rowVo2(ViewRow& row, const ScanData& scan)
{
    const ScanFile& f = scan.files[kIdxVo2];
    if (!f.r.exists) {
        setRow(row, Light::Grey, "VO2: no file");
        return;
    }
    if (!scan.vo2Ok) {
        setRow(row, Light::Amber, "VO2: no data");
        return;
    }
    const RunVo2::Glance::View& v = scan.vo2;
    char t[kRowChars + 16];
    if (v.beforeX10 != 0) {
        const int d = static_cast<int>(v.vo2x10) - static_cast<int>(v.beforeX10);
        const int a = d < 0 ? -d : d;
        std::snprintf(t, sizeof(t), "VO2 %u.%u (%c%d.%d)", static_cast<unsigned>(v.vo2x10 / 10u),
                      static_cast<unsigned>(v.vo2x10 % 10u), d < 0 ? '-' : '+', a / 10, a % 10);
    } else {
        std::snprintf(t, sizeof(t), "VO2 %u.%u", static_cast<unsigned>(v.vo2x10 / 10u),
                      static_cast<unsigned>(v.vo2x10 % 10u));
    }
    setRow(row, Light::Green, t);
}

void ringFromWeek(ViewData& view, const WeekView& week)
{
    for (uint8_t i = 0; i < kRingDays; ++i) {
        view.level[i] = static_cast<uint8_t>(week.day[i].level);
    }
    view.today = week.today;
}

void trendFromVo2(ViewData& view, const ScanData& scan)
{
    view.trendCount = 0;
    if (!scan.vo2Ok) {
        return;
    }
    const RunVo2::Glance::View& v = scan.vo2;
    uint8_t n = v.count < kTrendMax ? v.count : kTrendMax;
    const uint8_t first = static_cast<uint8_t>(v.count - n);
    uint16_t lo = 0xFFFF, hi = 0;
    for (uint8_t i = 0; i < n; ++i) {
        const uint16_t p = v.trend[first + i];
        lo = p < lo ? p : lo;
        hi = p > hi ? p : hi;
    }
    for (uint8_t i = 0; i < n; ++i) {
        const uint16_t p = v.trend[first + i];
        view.trend[i] = (hi == lo) ? 11 : static_cast<uint8_t>(2u + (static_cast<uint32_t>(p - lo) * 18u) / (hi - lo));
    }
    view.trendCount = n;
}

} // namespace ClockProbe
