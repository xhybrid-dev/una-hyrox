/**
 ******************************************************************************
 * @file    PlanState.cpp
 * @brief   plan.json. See PlanState.hpp.
 ******************************************************************************
 */

#include "PlanState.hpp"

#include "PaceModel.hpp"
#include "RunVo2Max.hpp"

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include <cstdio>
#include <cstring>

namespace XTraining {

namespace {

constexpr uint8_t kSetupFields = 12;
constexpr uint8_t kCalendarFields = 4;
constexpr uint8_t kLogFields = 4;

bool getAt(SDK::JsonStreamReader& r, const char* array, int i, int j, int64_t& out)
{
    char q[24];
    if (j < 0) {
        std::snprintf(q, sizeof(q), "%s[%d]", array, i);
    } else {
        std::snprintf(q, sizeof(q), "%s[%d][%d]", array, i, j);
    }
    return r.get(q, out);
}

bool inRange(int64_t v, int64_t lo, int64_t hi) { return v >= lo && v <= hi; }

bool dateOrZero(int64_t v) { return v == 0 || (inRange(v, 0, 0xFFFFFFFFll) && Date::valid(static_cast<Ymd>(v))); }

}  // namespace

void PlanState::record(const LogEntry& e)
{
    for (uint8_t i = 0; i < mCount; ++i) {
        if (mLog[i].date == e.date) {
            mLog[i] = e;
            return;
        }
    }
    if (mCount == kMaxLog) {
        // Keep the newest. The log is in date order, so the oldest is first.
        if (e.date < mLog[0].date) {
            return;
        }
        for (uint8_t i = 1; i < mCount; ++i) {
            mLog[i - 1] = mLog[i];
        }
        --mCount;
    }
    // Insert in date order.
    uint8_t at = mCount;
    while (at > 0 && mLog[at - 1].date > e.date) {
        mLog[at] = mLog[at - 1];
        --at;
    }
    mLog[at] = e;
    ++mCount;
}

void PlanState::setOverride(const Override& o)
{
    for (uint8_t i = 0; i < overrideCount; ++i) {
        if (overrides[i].date == o.date) {
            overrides[i] = o;
            return;
        }
    }
    if (overrideCount == kMaxOverrides) {
        for (uint8_t i = 1; i < overrideCount; ++i) {
            overrides[i - 1] = overrides[i];
        }
        --overrideCount;
    }
    overrides[overrideCount++] = o;
}

const Override* PlanState::findOverride(Ymd date) const
{
    for (uint8_t i = 0; i < overrideCount; ++i) {
        if (overrides[i].date == date) {
            return &overrides[i];
        }
    }
    return nullptr;
}

void PlanState::pruneOverrides(Ymd date)
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < overrideCount; ++i) {
        if (overrides[i].date >= date) {
            overrides[n++] = overrides[i];
        }
    }
    overrideCount = n;
}

const LogEntry* PlanState::find(Ymd date) const
{
    for (uint8_t i = 0; i < mCount; ++i) {
        if (mLog[i].date == date) {
            return &mLog[i];
        }
    }
    return nullptr;
}

size_t PlanState::toJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(1));
    w.startArray("s");
    w.add(static_cast<uint8_t>(setup.plan));
    w.add(setup.level);
    w.add(static_cast<uint8_t>(setup.days));
    w.add(setup.longDay);
    w.add(static_cast<uint8_t>(setup.easyByHr ? 1 : 0));
    w.add(static_cast<uint8_t>(setup.repsByDistance ? 1 : 0));
    w.add(static_cast<uint32_t>(setup.raceDate));
    w.add(setup.fiveKSec);
    w.add(static_cast<uint32_t>(setup.fiveKDate));
    w.add(static_cast<uint8_t>(setup.fiveKRecent ? 1 : 0));
    w.add(setup.halfSec);
    w.add(static_cast<uint32_t>(setup.halfDate));
    w.endArray();
    w.startArray("c");
    w.add(static_cast<uint8_t>(calendar.status));
    w.add(static_cast<uint32_t>(calendar.start));
    w.add(calendar.firstWeek);
    w.add(calendar.weeks);
    w.endArray();
    w.startArray("log");
    for (uint8_t i = 0; i < mCount; ++i) {
        w.startArray();
        w.add(static_cast<uint32_t>(mLog[i].date));
        w.add(static_cast<uint8_t>(mLog[i].kind));
        w.add(static_cast<uint8_t>(mLog[i].status));
        w.add(mLog[i].startUtc);
        w.endArray();
    }
    w.endArray();
    if (vo2StartX10 != 0) {
        w.add("vo2s", vo2StartX10);
    }
    if (calendar.adjCount > 0) {
        w.startArray("adj");
        for (uint8_t i = 0; i < calendar.adjCount; ++i) {
            w.startArray();
            w.add(calendar.adj[i].runWeek);
            w.add(static_cast<uint8_t>(calendar.adj[i].delta + 8));
            w.endArray();
        }
        w.endArray();
    }
    if (overrideCount > 0) {
        w.startArray("ov");
        for (uint8_t i = 0; i < overrideCount; ++i) {
            w.startArray();
            w.add(static_cast<uint32_t>(overrides[i].date));
            w.add(static_cast<uint32_t>(overrides[i].from));
            w.add(static_cast<uint8_t>(overrides[i].mode));
            w.endArray();
        }
        w.endArray();
    }
    if (pausedFrom != 0) {
        w.add("ps", static_cast<uint32_t>(pausedFrom));
    }
    if (retestFrom != 0) {
        w.add("rt", static_cast<uint32_t>(retestFrom));
    }
    if (resumeWeek != 0) {
        w.add("rw", resumeWeek);
    }
    if (settledTo != 0) {
        w.add("st", static_cast<uint32_t>(settledTo));
    }
    if (askFiveKSec != 0) {
        w.add("ak", askFiveKSec);
    }
    if (offerDeclined != 0) {
        w.add("od", static_cast<uint32_t>(offerDeclined));
    }
    w.endMap();
    if (w.isError()) {
        buf[0] = '\0';
        return 0;
    }
    return std::strlen(buf);
}

bool PlanState::fromJson(const char* text, size_t len)
{
    clear();
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
    size_t n = 0;
    if (!r.getArrayLength("s", n) || n != kSetupFields) {
        return false;
    }
    int64_t s[kSetupFields] = {};
    for (uint8_t i = 0; i < kSetupFields; ++i) {
        if (!getAt(r, "s", i, -1, s[i])) {
            return false;
        }
    }
    PlanState st;
    if (!inRange(s[0], 0, 3) || !inRange(s[1], 1, 3) || !inRange(s[2], 0, 127) || !inRange(s[3], 0, 6)
        || !inRange(s[4], 0, 1) || !inRange(s[5], 0, 1) || !dateOrZero(s[6])
        || !(s[7] == 0 || inRange(s[7], PaceConfig::kMin5kSec, PaceConfig::kMax5kSec)) || !dateOrZero(s[8])
        || !inRange(s[9], 0, 1) || !(s[10] == 0 || inRange(s[10], PaceConfig::kMinHalfSec, PaceConfig::kMaxHalfSec))
        || !dateOrZero(s[11])) {
        return false;
    }
    st.setup.plan = static_cast<PlanId>(s[0]);
    st.setup.level = static_cast<uint8_t>(s[1]);
    st.setup.days = static_cast<DayMask>(s[2]);
    st.setup.longDay = static_cast<uint8_t>(s[3]);
    st.setup.easyByHr = s[4] == 1;
    st.setup.repsByDistance = s[5] == 1;
    st.setup.raceDate = static_cast<Ymd>(s[6]);
    st.setup.fiveKSec = static_cast<uint32_t>(s[7]);
    st.setup.fiveKDate = static_cast<Ymd>(s[8]);
    st.setup.fiveKRecent = s[9] == 1;
    st.setup.halfSec = static_cast<uint32_t>(s[10]);
    st.setup.halfDate = static_cast<Ymd>(s[11]);
    if (st.setup.plan != PlanId::None && !WeekLayout::valid(st.setup.days, st.setup.longDay)) {
        return false;
    }

    if (!r.getArrayLength("c", n) || n != kCalendarFields) {
        return false;
    }
    int64_t c[kCalendarFields] = {};
    for (uint8_t i = 0; i < kCalendarFields; ++i) {
        if (!getAt(r, "c", i, -1, c[i])) {
            return false;
        }
    }
    if (!inRange(c[0], 0, static_cast<int64_t>(Calendar::Status::BadDate)) || !dateOrZero(c[1])
        || !inRange(c[2], 0, 16) || !inRange(c[3], 0, 20) || c[2] + c[3] > 20) {
        return false;
    }
    st.calendar.status = static_cast<Calendar::Status>(c[0]);
    st.calendar.start = static_cast<Ymd>(c[1]);
    st.calendar.firstWeek = static_cast<uint8_t>(c[2]);
    st.calendar.weeks = static_cast<uint8_t>(c[3]);
    if (st.calendar.usable() && (st.calendar.start == 0 || st.calendar.weeks == 0
                                 || Date::weekday(st.calendar.start) != Weekday::Mon)) {
        return false;
    }

    if (!r.getArrayLength("log", n) || n > kMaxLog) {
        return false;
    }
    Ymd last = 0;
    for (uint8_t i = 0; i < n; ++i) {
        char q[16];
        size_t fields = 0;
        std::snprintf(q, sizeof(q), "log[%u]", static_cast<unsigned>(i));
        if (!r.getArrayLength(q, fields) || fields != kLogFields) {
            return false;
        }
        int64_t e[kLogFields] = {};
        for (uint8_t j = 0; j < kLogFields; ++j) {
            if (!getAt(r, "log", i, j, e[j])) {
                return false;
            }
        }
        if (!dateOrZero(e[0]) || e[0] == 0 || static_cast<Ymd>(e[0]) <= last || !inRange(e[1], 0, kKindCount - 1)
            || !inRange(e[2], 1, 4) || !inRange(e[3], 0, 0xFFFFFFFFll)) {
            return false;
        }
        last = static_cast<Ymd>(e[0]);
        st.mLog[i] = LogEntry{last, static_cast<Kind>(e[1]), static_cast<DayStatus>(e[2]),
                              static_cast<uint32_t>(e[3])};
    }
    st.mCount = static_cast<uint8_t>(n);

    // TP4: X Run's VO2max as first seen in this plan. Optional; checked when there.
    int64_t vo2s = 0;
    if (r.get("vo2s", vo2s)) {
        if (!inRange(vo2s, RunVo2Max::kMinX10, RunVo2Max::kMaxX10)) {
            return false;
        }
        st.vo2StartX10 = static_cast<uint16_t>(vo2s);
    }

    // TP5: the plan's memory, each optional and checked when there.
    size_t an = 0;
    if (r.getArrayLength("adj", an)) {
        if (an > kMaxAdjust) {
            return false;
        }
        int prev = -1;
        for (size_t i = 0; i < an; ++i) {
            int64_t rw = 0, d = 0;
            if (!getAt(r, "adj", static_cast<int>(i), 0, rw) || !getAt(r, "adj", static_cast<int>(i), 1, d)
                || !inRange(rw, 0, 20) || !inRange(d, 0, 16) || static_cast<int>(rw) < prev) {
                return false;
            }
            prev = static_cast<int>(rw);
            st.calendar.adj[i] = WeekAdjust{static_cast<uint8_t>(rw), static_cast<int8_t>(d - 8)};
        }
        st.calendar.adjCount = static_cast<uint8_t>(an);
    }
    if (r.getArrayLength("ov", an)) {
        if (an > kMaxOverrides) {
            return false;
        }
        for (size_t i = 0; i < an; ++i) {
            int64_t d = 0, f = 0, m = 0;
            if (!getAt(r, "ov", static_cast<int>(i), 0, d) || !getAt(r, "ov", static_cast<int>(i), 1, f)
                || !getAt(r, "ov", static_cast<int>(i), 2, m) || !dateOrZero(d) || d == 0 || !dateOrZero(f)
                || !inRange(m, 1, 3) || (m == 1 && f == 0)) {
                return false;
            }
            st.overrides[i] = Override{static_cast<Ymd>(d), static_cast<Ymd>(f), static_cast<OverrideMode>(m)};
        }
        st.overrideCount = static_cast<uint8_t>(an);
    }
    int64_t dv = 0;
    if (r.get("ps", dv)) {
        if (!dateOrZero(dv)) {
            return false;
        }
        st.pausedFrom = static_cast<Ymd>(dv);
    }
    if (r.get("rt", dv)) {
        if (!dateOrZero(dv)) {
            return false;
        }
        st.retestFrom = static_cast<Ymd>(dv);
    }
    if (r.get("rw", dv)) {
        if (!inRange(dv, 0, 20)) {
            return false;
        }
        st.resumeWeek = static_cast<uint8_t>(dv);
    }
    if (r.get("st", dv)) {
        if (!dateOrZero(dv)) {
            return false;
        }
        st.settledTo = static_cast<Ymd>(dv);
    }
    if (r.get("od", dv)) {
        if (!dateOrZero(dv)) {
            return false;
        }
        st.offerDeclined = static_cast<Ymd>(dv);
    }
    if (r.get("ak", dv)) {
        if (!inRange(dv, PaceConfig::kMin5kSec, PaceConfig::kMax5kSec)) {
            return false;
        }
        st.askFiveKSec = static_cast<uint32_t>(dv);
    }
    *this = st;
    return true;
}

}  // namespace XTraining
