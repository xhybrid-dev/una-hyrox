/**
 ******************************************************************************
 * @file    Vo2History.cpp
 * @brief   The VO2max history ring, the run maxima, and their JSON.
 ******************************************************************************
 */

#include "Vo2History.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include "Vo2Profile.hpp"

namespace RunVo2
{

namespace
{

/// The SDK reader's integer getters do no type or range check (strtoul on
/// whatever is there), so read as int64 and bound it here.
bool readInt(const SDK::JsonStreamReader& r, const char* query, int64_t min, int64_t max, int64_t& out)
{
    int64_t v = 0;
    if (!r.get(query, v) || v < min || v > max) {
        return false;
    }
    out = v;
    return true;
}

}  // namespace

void Vo2History::add(const RunRecord& r)
{
    if (mCount < Config::kHistoryRuns) {
        mRuns[(mFirst + mCount) % Config::kHistoryRuns] = r;
        ++mCount;
    } else {
        mRuns[mFirst] = r;
        mFirst = static_cast<uint8_t>((mFirst + 1) % Config::kHistoryRuns);
    }
    RunRecord& added = mRuns[(mFirst + mCount - 1) % Config::kHistoryRuns];
    if (added.shownX10 == 0) {
        added.shownX10 = rollingOver(mCount);
    }
}

void Vo2History::addSorted(const RunRecord& r)
{
    RunRecord tmp[Config::kHistoryRuns + 1];
    uint8_t   n      = 0;
    uint8_t   placed = Config::kHistoryRuns + 1;   // where r went in tmp
    for (uint8_t i = 0; i < mCount; ++i) {
        const RunRecord& e = at(i);
        if (placed > n && r.utc < e.utc) {
            placed   = n;
            tmp[n++] = r;
        }
        tmp[n++] = e;
    }
    if (placed > n) {
        placed   = n;
        tmp[n++] = r;
    }
    const uint8_t skip = n > Config::kHistoryRuns ? static_cast<uint8_t>(n - Config::kHistoryRuns) : 0;
    mFirst = 0;
    mCount = static_cast<uint8_t>(n - skip);
    for (uint8_t i = 0; i < mCount; ++i) {
        mRuns[i] = tmp[skip + i];
    }
    // The value shown after r, and after every run after it, has changed.
    // (If r was older than all ten kept it was dropped, and nothing has.)
    if (placed >= skip) {
        for (uint8_t i = static_cast<uint8_t>(placed - skip); i < mCount; ++i) {
            mRuns[i].shownX10 = rollingOver(static_cast<uint8_t>(i + 1));
        }
    }
}

void Vo2History::setRun(uint8_t i, uint16_t vo2x10, uint16_t windows)
{
    if (i < mCount) {
        RunRecord& r = mRuns[(mFirst + i) % Config::kHistoryRuns];
        r.vo2x10  = vo2x10;
        r.windows = windows;
    }
}

void Vo2History::removeAt(uint8_t i)
{
    if (i >= mCount) {
        return;
    }
    RunRecord tmp[Config::kHistoryRuns];
    uint8_t   n = 0;
    for (uint8_t k = 0; k < mCount; ++k) {
        if (k != i) {
            tmp[n++] = at(k);
        }
    }
    mFirst = 0;
    mCount = n;
    for (uint8_t k = 0; k < n; ++k) {
        mRuns[k] = tmp[k];
    }
}

void Vo2History::recomputeShown()
{
    for (uint8_t i = 0; i < mCount; ++i) {
        mRuns[(mFirst + i) % Config::kHistoryRuns].shownX10 = rollingOver(static_cast<uint8_t>(i + 1));
    }
}

uint8_t Vo2History::countFrom(RunSource src) const
{
    uint8_t n = 0;
    for (uint8_t i = 0; i < mCount; ++i) {
        if (at(i).source == static_cast<uint8_t>(src)) {
            ++n;
        }
    }
    return n;
}

const RunRecord& Vo2History::at(uint8_t i) const
{
    static const RunRecord kNone{};
    if (i >= mCount) {
        return kNone;
    }
    return mRuns[(mFirst + i) % Config::kHistoryRuns];
}

uint16_t Vo2History::rollingX10Within(uint32_t nowUtc) const
{
    const uint32_t span = static_cast<uint32_t>(Config::kMaxRunAgeDays) * 86400u;
    return rollingOver(mCount, nowUtc > span ? nowUtc - span : 1u);
}

uint16_t Vo2History::rollingOver(uint8_t n, uint32_t notBeforeUtc) const
{
    uint32_t sum = 0;
    uint32_t weight = 0;
    if (n > mCount) {
        n = mCount;
    }
    const uint8_t from = n > Config::kRollingRuns ? n - Config::kRollingRuns : 0;
    // The age limit, anchored on the latest of these n runs (Vo2Config kMaxRunAgeDays), unless the
    // caller gave its own (rollingX10Within).
    if (notBeforeUtc == 0 && Config::kAgeLimitEnabled && n > 0) {
        const uint32_t span   = static_cast<uint32_t>(Config::kMaxRunAgeDays) * 86400u;
        const uint32_t latest = at(static_cast<uint8_t>(n - 1)).utc;
        notBeforeUtc = latest > span ? latest - span : 0;
    }
    for (uint8_t i = from; i < n; ++i) {
        const RunRecord& r = at(i);
        if (notBeforeUtc != 0 && r.utc < notBeforeUtc) {
            continue;   // older than the age limit
        }
        const uint32_t w = r.windows < Config::kMaxRunWeight ? r.windows : Config::kMaxRunWeight;
        sum    += static_cast<uint32_t>(r.vo2x10) * w;
        weight += w;
    }
    if (weight == 0) {
        return 0;
    }
    return static_cast<uint16_t>((sum + weight / 2) / weight);
}

void Vo2History::addRunMax(uint8_t bpm)
{
    if (bpm < Config::kMinHrBpm || bpm > Config::kMaxHrBpm) {
        return;
    }
    if (mMaxCount < Config::kHistoryRuns) {
        mMaxes[(mMaxFirst + mMaxCount) % Config::kHistoryRuns] = bpm;
        ++mMaxCount;
    } else {
        mMaxes[mMaxFirst] = bpm;
        mMaxFirst = static_cast<uint8_t>((mMaxFirst + 1) % Config::kHistoryRuns);
    }
}

uint8_t Vo2History::autoMaxHr() const
{
    static_assert(Config::kAutoMaxConfirmRuns >= 1, "at least one run");
    if (mMaxCount < Config::kAutoMaxConfirmRuns) {
        return 0;
    }
    // The k-th highest: k passes, each taking the highest left below the last.
    uint8_t  value = 255;
    uint8_t  taken = 0;
    while (taken < Config::kAutoMaxConfirmRuns) {
        uint8_t best = 0;
        uint8_t copies = 0;
        for (uint8_t i = 0; i < mMaxCount; ++i) {
            const uint8_t m = mMaxes[i];
            if (m < value && m > best) {
                best = m;
                copies = 1;
            } else if (m == best && m < value) {
                ++copies;
            }
        }
        if (best == 0) {
            return 0;   // cannot happen with mMaxCount >= k; kept as a bound
        }
        value = best;
        taken = static_cast<uint8_t>(taken + copies);
    }
    return value;
}

void Vo2History::setWatchRestHr(uint8_t bpm)
{
    if (plausibleRestHr(bpm)) {
        mWatchRestHr = bpm;
    }
}

size_t Vo2History::toJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(4));
    w.add("restHr", mWatchRestHr);
    w.add("trail", mTrailMark);
    w.add("pk", mProfileKey);   // read only if present: older files score once
    w.startArray("maxes");
    for (uint8_t i = 0; i < mMaxCount; ++i) {
        w.add(mMaxes[(mMaxFirst + i) % Config::kHistoryRuns]);
    }
    w.endArray();
    w.startArray("runs");
    for (uint8_t i = 0; i < mCount; ++i) {
        const RunRecord& r = at(i);
        w.startArray();
        w.add(r.utc);
        w.add(r.vo2x10);
        w.add(r.windows);
        w.add(r.source);
        w.add(r.shownX10);
        w.endArray();
    }
    w.endArray();
    w.endMap();
    if (w.isError()) {
        buf[0] = '\0';
        return 0;
    }
    return std::strlen(buf);
}

bool Vo2History::fromJson(const char* text, size_t len)
{
    clear();
    if (text == nullptr || len == 0) {
        return false;
    }
    SDK::JsonStreamReader r(text, len);
    if (!r.validate()) {
        return false;
    }

    Vo2History h;
    int64_t v = 0;
    if (!readInt(r, "v", 1, 4, v)) {
        return false;
    }
    const int64_t version = v;

    if (version == 1) {
        // R1-R3 kept one ratcheting value; it becomes one run's maximum, which
        // alone never confirms (kAutoMaxConfirmRuns).
        if (!readInt(r, "autoMaxHr", 0, Config::kMaxHrBpm, v)) {
            return false;
        }
        h.addRunMax(static_cast<uint8_t>(v));
    } else {
        if (!readInt(r, "restHr", 0, 255, v)) {
            return false;
        }
        h.setWatchRestHr(static_cast<uint8_t>(v));
        if (version >= 3) {
            if (!readInt(r, "trail", 0, 0xFFFFFFFFll, v)) {
                return false;
            }
            h.setTrailMarkUtc(static_cast<uint32_t>(v));
        }
        int64_t pk = 0;
        if (r.get("pk", pk)) {   // optional: absent before re-scoring
            if (pk < 0 || pk > 0xFFFFFFFFll) {
                return false;
            }
            h.setProfileKey(static_cast<uint32_t>(pk));
        }

        size_t n = 0;
        if (!r.getArrayLength("maxes", n)) {
            return false;
        }
        for (size_t i = 0; i < n; ++i) {
            char q[24];
            std::snprintf(q, sizeof(q), "maxes[%u]", static_cast<unsigned>(i));
            if (!readInt(r, q, Config::kMinHrBpm, Config::kMaxHrBpm, v)) {
                return false;
            }
            h.addRunMax(static_cast<uint8_t>(v));   // keeps the newest kHistoryRuns
        }
    }

    size_t runs = 0;
    if (!r.getArrayLength("runs", runs)) {
        return false;
    }
    for (size_t i = 0; i < runs; ++i) {
        char q[24];
        int64_t utc = 0, vo2 = 0, win = 0, src = 0, shown = 0;
        std::snprintf(q, sizeof(q), "runs[%u][0]", static_cast<unsigned>(i));
        bool ok = readInt(r, q, 1, 0xFFFFFFFFll, utc);
        std::snprintf(q, sizeof(q), "runs[%u][1]", static_cast<unsigned>(i));
        ok = ok && readInt(r, q, static_cast<int64_t>(Config::kMinPlausibleVo2 * 10.0f),
                           static_cast<int64_t>(Config::kMaxPlausibleVo2 * 10.0f), vo2);
        std::snprintf(q, sizeof(q), "runs[%u][2]", static_cast<unsigned>(i));
        ok = ok && readInt(r, q, Config::kMinWindows, Config::kMaxWindows, win);
        // Numbers a run has, by version: 1 and 2 three (the 17 October build's
        // version 2 four, the fourth the value shown after the run), 3 four
        // (the fourth the source), 4 five (source, then the value shown).
        // The value shown (R6) is worked out when the run is added if the
        // file does not have it.
        size_t fields = 0;
        std::snprintf(q, sizeof(q), "runs[%u]", static_cast<unsigned>(i));
        ok = ok && r.getArrayLength(q, fields);
        const size_t minFields = version >= 4 ? 5 : (version == 3 ? 4 : 3);
        const size_t maxFields = version >= 4 ? 5 : (version == 3 ? 4 : (version == 2 ? 4 : 3));
        ok = ok && fields >= minFields && fields <= maxFields;
        size_t shownAt = 99;   // the index of the shown value, if it has one
        if (ok && version >= 3) {
            std::snprintf(q, sizeof(q), "runs[%u][3]", static_cast<unsigned>(i));
            ok = readInt(r, q, 0, 1, src);
            shownAt = 4;
        } else if (version == 2) {
            shownAt = 3;
        }
        if (ok && fields > shownAt) {
            std::snprintf(q, sizeof(q), "runs[%u][%u]", static_cast<unsigned>(i), static_cast<unsigned>(shownAt));
            ok = readInt(r, q, static_cast<int64_t>(Config::kMinPlausibleVo2 * 10.0f),
                         static_cast<int64_t>(Config::kMaxPlausibleVo2 * 10.0f), shown);
        }
        if (!ok) {
            return false;
        }
        h.add(RunRecord{static_cast<uint32_t>(utc), static_cast<uint16_t>(vo2), static_cast<uint16_t>(win),
                        static_cast<uint8_t>(src), static_cast<uint16_t>(shown)});
    }

    *this = h;
    return true;
}

uint8_t Vo2History::trendX10(uint16_t* out, uint8_t max) const
{
    if (out == nullptr) {
        return 0;
    }
    const uint8_t n = mCount < max ? mCount : max;
    const uint8_t from = static_cast<uint8_t>(mCount - n);
    for (uint8_t i = 0; i < n; ++i) {
        out[i] = at(static_cast<uint8_t>(from + i)).shownX10;
    }
    return n;
}

size_t Vo2History::toSharedJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    const uint32_t last = mCount ? at(static_cast<uint8_t>(mCount - 1)).utc : 0;
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(2));
    w.add("vo2maxX10", rollingX10());
    w.add("runs", mCount);
    w.add("utc", last);
    w.add("beforeX10", rollingX10Before());
    uint16_t trend[Config::kHistoryRuns];
    const uint8_t n = trendX10(trend, Config::kHistoryRuns);
    w.startArray("trend");
    for (uint8_t i = 0; i < n; ++i) {
        w.add(trend[i]);
    }
    w.endArray();
    w.endMap();
    if (w.isError()) {
        buf[0] = '\0';
        return 0;
    }
    return std::strlen(buf);
}

}  // namespace RunVo2
