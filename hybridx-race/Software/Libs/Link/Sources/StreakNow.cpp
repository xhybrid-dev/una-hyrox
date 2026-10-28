/**
 ******************************************************************************
 * @file    StreakNow.cpp
 * @brief   The Streak line (see the header).
 ******************************************************************************
 */

#include "StreakNow.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

namespace HybridXLink
{

namespace
{

constexpr uint32_t kSecondsDay = 86400u;

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

uint32_t appKey(const char* folder)
{
    uint32_t h = 2166136261u;
    for (const char* p = folder; p && *p; ++p) {
        h ^= static_cast<uint8_t>(*p);
        h *= 16777619u;
    }
    return h;
}

uint8_t kindCode(Sport s)
{
    switch (s) {
        case Sport::Run:      return 0;
        case Sport::Ride:     return 1;
        case Sport::Walk:     return 2;
        case Sport::Strength: return 3;
        case Sport::Workout:  return 4;
        case Sport::Hybrid:   return 5;
        case Sport::Row:      return 6;
        case Sport::Other:    break;
    }
    return 7;
}

size_t encodeStreakNow(const StreakNow& s, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    if (s.day < 0 || s.weekStart < 0 || s.keyCount > kMaxKeys || s.target == 0) {
        return 0;
    }
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(1));
    w.add("day", static_cast<uint32_t>(s.day));
    w.add("ws", static_cast<uint32_t>(s.weekStart));
    w.add("t", s.target);
    w.add("min", s.minMinutes);
    w.add("sc", s.scope);
    w.add("opd", static_cast<uint8_t>(s.onePerDay ? 1 : 0));
    w.add("pend", static_cast<uint8_t>(s.pending ? 1 : 0));
    w.add("n", s.counted);
    w.add("days", s.daysMask);
    w.add("st", s.streak);
    w.startArray("k");
    for (uint8_t i = 0; i < s.keyCount; ++i) {
        w.startArray();
        w.add(s.keys[i].appKey);
        w.add(s.keys[i].localStart);
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

bool decodeStreakNow(const char* text, size_t len, StreakNow& out)
{
    out = StreakNow{};
    if (text == nullptr || len == 0) {
        return false;
    }
    SDK::JsonStreamReader r(text, len);
    if (!r.validate()) {
        return false;
    }
    StreakNow s;
    int64_t v = 0;
    if (!readInt(r, "v", 1, 1, v)) {
        return false;
    }
    // Days since 1970: up to 2100 is about 47,500.
    constexpr int64_t kMaxDay = 50000;
    bool ok = readInt(r, "day", 0, kMaxDay, v);
    s.day = static_cast<int32_t>(v);
    ok = ok && readInt(r, "ws", 0, kMaxDay, v);
    s.weekStart = static_cast<int32_t>(v);
    ok = ok && s.weekStart <= s.day && s.day - s.weekStart < 7;
    ok = ok && readInt(r, "t", 1, 7, v);
    s.target = static_cast<uint8_t>(v);
    ok = ok && readInt(r, "min", 0, 240, v);
    s.minMinutes = static_cast<uint8_t>(v);
    ok = ok && readInt(r, "sc", 0, 255, v);
    s.scope = static_cast<uint8_t>(v);
    ok = ok && (s.scope == kScopeAny || s.scope < 8);
    ok = ok && readInt(r, "opd", 0, 1, v);
    s.onePerDay = v != 0;
    ok = ok && readInt(r, "pend", 0, 1, v);
    s.pending = v != 0;
    ok = ok && readInt(r, "n", 0, 255, v);
    s.counted = static_cast<uint8_t>(v);
    ok = ok && readInt(r, "days", 0, 127, v);
    s.daysMask = static_cast<uint8_t>(v);
    ok = ok && readInt(r, "st", 0, 65535, v);
    s.streak = static_cast<uint16_t>(v);
    size_t n = 0;
    ok = ok && r.getArrayLength("k", n) && n <= kMaxKeys;
    if (!ok) {
        return false;
    }
    for (size_t i = 0; i < n; ++i) {
        char q[16];
        size_t fields = 0;
        std::snprintf(q, sizeof(q), "k[%u]", static_cast<unsigned>(i));
        if (!r.getArrayLength(q, fields) || fields != 2) {
            return false;
        }
        int64_t key = 0, start = 0;
        std::snprintf(q, sizeof(q), "k[%u][0]", static_cast<unsigned>(i));
        if (!readInt(r, q, 0, 0xFFFFFFFFll, key)) {
            return false;
        }
        std::snprintf(q, sizeof(q), "k[%u][1]", static_cast<unsigned>(i));
        if (!readInt(r, q, 0, 0xFFFFFFFFll, start)) {
            return false;
        }
        s.keys[i].appKey     = static_cast<uint32_t>(key);
        s.keys[i].localStart = static_cast<uint32_t>(start);
    }
    s.keyCount = static_cast<uint8_t>(n);
    out = s;
    return true;
}

StreakProjector::StreakProjector(const StreakNow& now, int32_t today)
    : mNow(now)
{
    mValid = now.target > 0 && today >= now.weekStart;
    if (!mValid) {
        return;
    }
    const int32_t weeks = (today - now.weekStart) / 7;
    mSameWeek  = weeks == 0;
    mNextWeek  = weeks == 1;
    mWeekStart = now.weekStart + weeks * 7;
    if (mSameWeek) {
        mDone = now.counted;
        mDays = now.daysMask;
    }
}

void StreakProjector::add(const ActivityFile& file)
{
    if (!mValid) {
        return;
    }
    const uint32_t key = appKey(file.app);
    for (uint8_t i = 0; i < file.count && i < kMaxEntries; ++i) {
        offer(key, file.entries[i]);
    }
}

void StreakProjector::offer(uint32_t key, const Entry& e)
{
    const uint32_t local = e.localStart();
    const int32_t  day   = static_cast<int32_t>(local / kSecondsDay);
    if (day < mWeekStart || day > mWeekStart + 6) {
        return;   // another week
    }
    if (mSameWeek) {
        for (uint8_t i = 0; i < mNow.keyCount; ++i) {
            if (mNow.keys[i].appKey == key && mNow.keys[i].localStart == local) {
                return;   // Streak has it already
            }
        }
    }
    for (uint8_t i = 0; i < mAddedCount; ++i) {
        if (mAdded[i].appKey == key && mAdded[i].localStart == local) {
            return;
        }
    }
    if (mAddedCount < sizeof(mAdded) / sizeof(mAdded[0])) {
        mAdded[mAddedCount++] = StreakKey{key, local};
    }

    // Streak's rules (StreakModel::status), in its order. The goal of a new week is this one's
    // unless a change waits, and then the line shows no target (result()).
    const uint32_t minutes = e.durationSec / 60u;   // whole minutes, as Streak's reader
    if (mNow.minMinutes > 0 && minutes < mNow.minMinutes) {
        return;
    }
    if (mNow.scope != kScopeAny && kindCode(e.sport) != mNow.scope) {
        return;
    }
    const uint8_t bit = static_cast<uint8_t>(1u << (day - mWeekStart));
    if (mNow.onePerDay && (mDays & bit)) {
        return;
    }
    mDays = static_cast<uint8_t>(mDays | bit);
    if (mDone < 255) {
        ++mDone;
    }
}

StreakLine StreakProjector::result() const
{
    StreakLine l;
    if (!mValid) {
        return l;
    }
    l.show = true;
    l.done = mDone;
    const bool goalKnown = mSameWeek || !mNow.pending;
    l.target = goalKnown ? mNow.target : 0;
    const bool met = goalKnown && mDone >= mNow.target;
    if (mSameWeek) {
        l.streak    = static_cast<uint16_t>(mNow.streak + (met ? 1 : 0));
        l.hasStreak = l.streak > 0;
    } else if (mNextWeek && mNow.counted >= mNow.target && goalKnown) {
        // Last week was met when Streak last saw it, so it is in the streak whatever else.
        l.streak    = static_cast<uint16_t>(mNow.streak + 1 + (met ? 1 : 0));
        l.hasStreak = true;
    }
    return l;
}

bool formatStreakLine(const StreakLine& l, char* line1, size_t cap1, char* line2, size_t cap2)
{
    if (line1 == nullptr || cap1 == 0 || line2 == nullptr || cap2 == 0) {
        return false;
    }
    line1[0] = '\0';
    line2[0] = '\0';
    if (!l.show) {
        return false;
    }
    if (l.target > 0) {
        std::snprintf(line1, cap1, "%u of %u this week", static_cast<unsigned>(l.done),
                      static_cast<unsigned>(l.target));
    } else {
        std::snprintf(line1, cap1, "%u this week", static_cast<unsigned>(l.done));
    }
    if (l.hasStreak) {
        std::snprintf(line2, cap2, "%u-week streak", static_cast<unsigned>(l.streak));
    }
    return true;
}

}  // namespace HybridXLink
