/**
 ******************************************************************************
 * @file    TodayFile.cpp
 * @brief   training_today.json. See TodayFile.hpp.
 ******************************************************************************
 */

#include "TodayFile.hpp"

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include <cstdio>
#include <cstring>

namespace XTraining {

const TodayDay* TodayData::find(Ymd date) const
{
    for (uint8_t i = 0; i < count; ++i) {
        if (day[i].date == date) {
            return &day[i];
        }
    }
    return nullptr;
}

namespace TodayFile {

namespace {

constexpr uint8_t kRowFields = 6;
constexpr int32_t kMaxWindowDays = TodayData::kMaxDays - 1;

bool inRange(int64_t v, int64_t lo, int64_t hi) { return v >= lo && v <= hi; }

bool dateOrZero(int64_t v) { return v == 0 || (inRange(v, 0, 0xFFFFFFFFll) && Date::valid(static_cast<Ymd>(v))); }

bool plainChar(char c) { return c >= 0x20 && c <= 0x7E && c != '"' && c != '\\'; }

/// A string from the file, into @p dst: printable ASCII only, and it must fit.
bool readText(const SDK::JsonStreamReader& r, const char* base, const char* query, char* dst, size_t cap)
{
    const char* p = nullptr;
    size_t n = 0;
    if (!r.get(query, p, n) || p == nullptr || n >= cap) {
        return false;
    }
    if (p <= base || p[-1] != '\"') {
        return false;   // a number or a literal where a string goes: the reader hands those back too
    }
    for (size_t i = 0; i < n; ++i) {
        if (!plainChar(p[i])) {
            return false;
        }
    }
    std::memcpy(dst, p, n);
    dst[n] = '\0';
    return true;
}

bool getRow(const SDK::JsonStreamReader& r, int i, int j, int64_t& out)
{
    char q[16];
    std::snprintf(q, sizeof(q), "d[%d][%d]", i, j);
    return r.get(q, out);
}

}  // namespace

void setText(char* dst, size_t cap, const char* src)
{
    if (dst == nullptr || cap == 0) {
        return;
    }
    size_t n = 0;
    for (; src != nullptr && *src != '\0' && n + 1 < cap; ++src) {
        if (plainChar(*src)) {
            dst[n++] = *src;
        }
    }
    dst[n] = '\0';
}

size_t write(const TodayData& data, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    if (data.count > TodayData::kMaxDays) {
        return 0;
    }
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(1));
    w.add("plan", data.plan);
    if (data.plan != 0) {
        w.add("wks", data.weeks);
        w.add("ps", static_cast<uint32_t>(data.pausedFrom));
        w.add("s", static_cast<uint32_t>(data.start));
        w.add("e", static_cast<uint32_t>(data.end));
        w.add("f", static_cast<uint32_t>(data.first));
        w.add("t", static_cast<uint32_t>(data.last));
        w.startArray("d");
        for (uint8_t i = 0; i < data.count; ++i) {
            const TodayDay& d = data.day[i];
            w.startArray();
            w.add(static_cast<uint32_t>(d.date));
            w.add(static_cast<uint8_t>(d.kind));
            w.add(static_cast<uint8_t>(d.status));
            w.add(d.runWeek);
            w.add(d.what);
            w.add(d.pace);
            w.endArray();
        }
        w.endArray();
    }
    w.endMap();
    if (w.isError()) {
        buf[0] = '\0';
        return 0;
    }
    return std::strlen(buf);
}

bool read(const char* text, size_t len, TodayData& out)
{
    out = TodayData{};
    if (text == nullptr || len == 0 || len > kMaxJsonBytes) {
        return false;
    }
    SDK::JsonStreamReader r(text, len);
    if (!r.validate()) {
        return false;
    }
    int64_t v = 0;
    if (!r.get("v", v) || v != 1) {
        return false;
    }
    TodayData d;
    if (!r.get("plan", v) || !inRange(v, 0, 3)) {
        return false;
    }
    d.plan = static_cast<uint8_t>(v);
    if (d.plan == 0) {
        out = d;   // no plan: nothing else to read
        return true;
    }
    int64_t dates[5] = {};
    if (!r.get("wks", v) || !inRange(v, 1, TodayData::kMaxWeeks)) {
        return false;
    }
    d.weeks = static_cast<uint8_t>(v);
    static const char* const kKeys[5] = {"ps", "s", "e", "f", "t"};
    for (int i = 0; i < 5; ++i) {
        if (!r.get(kKeys[i], dates[i]) || !dateOrZero(dates[i])) {
            return false;
        }
    }
    d.pausedFrom = static_cast<Ymd>(dates[0]);
    d.start      = static_cast<Ymd>(dates[1]);
    d.end        = static_cast<Ymd>(dates[2]);
    d.first      = static_cast<Ymd>(dates[3]);
    d.last       = static_cast<Ymd>(dates[4]);
    if (d.start == 0 || d.end == 0 || d.first == 0 || d.last == 0 || d.end < d.start || d.last < d.first
        || Date::toDayNum(d.last) - Date::toDayNum(d.first) > kMaxWindowDays) {
        return false;
    }

    size_t n = 0;
    if (!r.getArrayLength("d", n) || n > TodayData::kMaxDays) {
        return false;
    }
    for (size_t i = 0; i < n; ++i) {
        size_t m = 0;
        char q[16];
        std::snprintf(q, sizeof(q), "d[%u]", static_cast<unsigned>(i));
        if (!r.getArrayLength(q, m) || m != kRowFields) {
            return false;
        }
        int64_t f[4] = {};
        for (int j = 0; j < 4; ++j) {
            if (!getRow(r, static_cast<int>(i), j, f[j])) {
                return false;
            }
        }
        if (!inRange(f[0], 0, 0xFFFFFFFFll) || !Date::valid(static_cast<Ymd>(f[0])) || !inRange(f[1], 0, kKindCount - 1)
            || !inRange(f[2], 0, 4) || !inRange(f[3], 1, 255)) {
            return false;
        }
        TodayDay& row = d.day[i];
        row.date    = static_cast<Ymd>(f[0]);
        row.kind    = static_cast<Kind>(f[1]);
        row.status  = static_cast<TodayStatus>(f[2]);
        row.runWeek = static_cast<uint8_t>(f[3]);
        char q4[16];
        std::snprintf(q4, sizeof(q4), "d[%u][4]", static_cast<unsigned>(i));
        char q5[16];
        std::snprintf(q5, sizeof(q5), "d[%u][5]", static_cast<unsigned>(i));
        if (!readText(r, text, q4, row.what, sizeof(row.what))
            || !readText(r, text, q5, row.pace, sizeof(row.pace))) {
            return false;
        }
        // In date order, one row a day, inside the window and the plan.
        if (row.date < d.first || row.date > d.last || row.date < d.start || row.date > d.end
            || (i > 0 && row.date <= d.day[i - 1].date)) {
            return false;
        }
    }
    d.count = static_cast<uint8_t>(n);
    out = d;
    return true;
}

Pick pick(const TodayData* data, Ymd today)
{
    Pick p;
    if (data == nullptr) {
        p.state = State::NoFile;
        return p;
    }
    if (data->plan == 0) {
        p.state = State::NoPlan;
        return p;
    }
    if (today == 0 || !Date::valid(today)) {
        p.state = State::ClockUnset;
        return p;
    }
    if (today < data->first || today > data->last) {
        p.state = State::Stale;   // a file from a fortnight ago, or from the future
        return p;
    }
    if (data->pausedFrom != 0 && today >= data->pausedFrom) {
        p.state = State::Paused;
        return p;
    }
    if (today < data->start) {
        p.state = State::BeforeStart;
        return p;
    }
    if (today > data->end) {
        p.state = State::Complete;
        return p;
    }
    // The next session: the first row after today that is a session.
    for (uint8_t i = 0; i < data->count; ++i) {
        if (data->day[i].date > today && data->day[i].kind != Kind::Rest) {
            p.next = &data->day[i];
            break;
        }
    }
    p.today = data->find(today);
    if (p.today == nullptr) {
        p.state = State::Stale;   // a day of the plan the file has no row for: not a file to trust
        p.next  = nullptr;
        return p;
    }
    if (p.today->kind == Kind::Rest) {
        p.state = State::Rest;
    } else if (p.today->status == TodayStatus::Done || p.today->status == TodayStatus::PartDone) {
        p.state = State::Done;
    } else if (p.today->status == TodayStatus::Skipped) {
        p.state = State::Skipped;
    } else {
        p.state = State::Session;
    }
    return p;
}

}  // namespace TodayFile
}  // namespace XTraining
