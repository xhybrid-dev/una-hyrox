/**
 ******************************************************************************
 * @file    ReviewLog.cpp
 * @brief   reviews.json. See ReviewLog.hpp.
 ******************************************************************************
 */

#include "ReviewLog.hpp"

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include <cstdio>

namespace XTraining {

namespace {

constexpr uint8_t kRowFields = 8;
constexpr int kGapBias  = 600;   // stored gap = gap + 600 (0 to 1200)
constexpr int kFadeBias = 100;   // stored fade = fade + 100 (0 to 200)

bool inRange(int64_t v, int64_t lo, int64_t hi) { return v >= lo && v <= hi; }

const ReviewEntry kEmpty{};

}  // namespace

ReviewEntry ReviewEntry::from(Ymd date, Kind kind, const Review::Result& r, uint8_t feel)
{
    ReviewEntry e;
    e.date = date;
    e.kind = kind;
    e.feel = feel;
    if (r.judged && r.judgedSec != 0) {
        e.inPct     = r.inPct;
        e.fastPct   = r.fastPct;
        e.gapSecKm  = r.gapSecKm;
        e.fadeSecKm = r.fadeSecKm;
        e.judgedSec = r.judgedSec;
    }
    return e;
}

void ReviewLog::clear()
{
    for (ReviewEntry& e : mEntry) {
        e = ReviewEntry{};
    }
    mCount  = 0;
    handled = 0;
}

const ReviewEntry& ReviewLog::at(uint8_t i) const
{
    return i < mCount ? mEntry[i] : kEmpty;
}

const ReviewEntry* ReviewLog::find(Ymd date) const
{
    for (uint8_t i = 0; i < mCount; ++i) {
        if (mEntry[i].date == date) {
            return &mEntry[i];
        }
    }
    return nullptr;
}

void ReviewLog::record(const ReviewEntry& e)
{
    for (uint8_t i = 0; i < mCount; ++i) {
        if (mEntry[i].date == e.date) {
            ReviewEntry n = e;
            if (n.feel == 0) {
                n.feel = mEntry[i].feel;   // a rerun of the day keeps the answer already given
            }
            mEntry[i] = n;
            return;
        }
    }
    if (mCount == kMax) {
        if (e.date < mEntry[0].date) {
            return;   // older than everything kept
        }
        for (uint8_t i = 1; i < mCount; ++i) {
            mEntry[i - 1] = mEntry[i];
        }
        --mCount;
    }
    uint8_t at = mCount;
    while (at > 0 && mEntry[at - 1].date > e.date) {
        mEntry[at] = mEntry[at - 1];
        --at;
    }
    mEntry[at] = e;
    ++mCount;
}

bool ReviewLog::setFeel(Ymd date, uint8_t feel)
{
    if (feel >= Review::kFeelCount) {
        return false;
    }
    for (uint8_t i = 0; i < mCount; ++i) {
        if (mEntry[i].date == date) {
            mEntry[i].feel = feel;
            return true;
        }
    }
    return false;
}

size_t ReviewLog::toJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(1));
    if (handled != 0) {
        w.add("eh", static_cast<uint32_t>(handled));
    }
    w.startArray("e");
    for (uint8_t i = 0; i < mCount; ++i) {
        const ReviewEntry& e = mEntry[i];
        w.startArray();
        w.add(static_cast<uint32_t>(e.date));
        w.add(static_cast<uint8_t>(e.kind));
        w.add(e.inPct);
        w.add(e.fastPct);
        w.add(static_cast<uint16_t>(e.gapSecKm + kGapBias));      // no negative numbers in the file
        w.add(static_cast<uint8_t>(e.fadeSecKm + kFadeBias));
        w.add(e.judgedSec);
        w.add(e.feel);
        w.endArray();
    }
    w.endArray();
    w.endMap();
    if (w.isError()) {
        buf[0] = '\0';
        return 0;
    }
    size_t n = 0;
    while (buf[n] != '\0') {
        ++n;
    }
    return n;
}

bool ReviewLog::fromJson(const char* text, size_t len)
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
    ReviewLog in;
    if (r.get("eh", v)) {
        if (!inRange(v, 0, 0xFFFFFFFFll) || (v != 0 && !Date::valid(static_cast<Ymd>(v)))) {
            return false;
        }
        in.handled = static_cast<Ymd>(v);
    }
    size_t n = 0;
    if (!r.getArrayLength("e", n) || n > kMax) {
        return false;
    }
    Ymd previous = 0;
    for (size_t i = 0; i < n; ++i) {
        char q[32];
        std::snprintf(q, sizeof(q), "e[%u]", static_cast<unsigned>(i));
        size_t m = 0;
        if (!r.getArrayLength(q, m) || m != kRowFields) {
            return false;
        }
        int64_t f[kRowFields] = {};
        for (int j = 0; j < kRowFields; ++j) {
            std::snprintf(q, sizeof(q), "e[%u][%d]", static_cast<unsigned>(i), j);
            if (!r.get(q, f[j])) {
                return false;
            }
        }
        if (!inRange(f[0], 0, 0xFFFFFFFFll) || !Date::valid(static_cast<Ymd>(f[0])) || !inRange(f[1], 0, kKindCount - 1)
            || !inRange(f[2], 0, 100) || !inRange(f[3], 0, 100) || f[2] + f[3] > 100 || !inRange(f[4], 0, 2 * kGapBias)
            || !inRange(f[5], 0, 2 * kFadeBias) || !inRange(f[6], 0, 0xFFFF) || !inRange(f[7], 0, Review::kFeelCount - 1)) {
            return false;
        }
        const Ymd date = static_cast<Ymd>(f[0]);
        if (date <= previous) {
            return false;   // oldest first, one a day
        }
        previous = date;
        ReviewEntry e;
        e.date      = date;
        e.kind      = static_cast<Kind>(f[1]);
        e.inPct     = static_cast<uint8_t>(f[2]);
        e.fastPct   = static_cast<uint8_t>(f[3]);
        e.gapSecKm  = static_cast<int16_t>(f[4] - kGapBias);
        e.fadeSecKm = static_cast<int8_t>(f[5] - kFadeBias);
        e.judgedSec = static_cast<uint16_t>(f[6]);
        e.feel      = static_cast<uint8_t>(f[7]);
        in.mEntry[in.mCount++] = e;
    }
    *this = in;
    return true;
}

}  // namespace XTraining
