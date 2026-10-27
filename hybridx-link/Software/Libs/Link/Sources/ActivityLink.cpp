/**
 ******************************************************************************
 * @file    ActivityLink.cpp
 * @brief   The HybridX activity summary file (see ActivityLink.hpp).
 ******************************************************************************
 */

#include "ActivityLink.hpp"

#include <cstdio>
#include <cstring>
#include <string_view>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

namespace HybridXLink
{

namespace
{

constexpr unsigned kFields = 7;   // startUtc, tz + bias, duration, distance, avgHr, maxHr, sport

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

bool readEntry(const SDK::JsonStreamReader& r, unsigned i, Entry& out)
{
    // [startUtc, tz + bias, durationSec, distanceM, avgHr, maxHr, sport]
    static constexpr int64_t kMin[kFields] = {kMinStartUtc, kMinTzOffsetMin + kTzOffsetBias, 0, 0, 0, 0, 0};
    static constexpr int64_t kMax[kFields] = {0xFFFFFFFFll, kMaxTzOffsetMin + kTzOffsetBias, kMaxDurationSec,
                                              kMaxDistanceM, 255, 255, 255};
    int64_t f[kFields];
    for (unsigned k = 0; k < kFields; ++k) {
        char q[24];
        std::snprintf(q, sizeof(q), "a[%u][%u]", i, k);
        if (!readInt(r, q, kMin[k], kMax[k], f[k])) {
            return false;
        }
    }
    Entry e;
    e.startUtc    = static_cast<uint32_t>(f[0]);
    e.tzOffsetMin = static_cast<int16_t>(f[1] - kTzOffsetBias);
    e.durationSec = static_cast<uint32_t>(f[2]);
    e.distanceM   = static_cast<uint32_t>(f[3]);
    e.avgHr       = static_cast<uint8_t>(f[4]);
    e.maxHr       = static_cast<uint8_t>(f[5]);
    e.sport       = sportFromCode(f[6]);
    if (!valid(e)) {
        return false;
    }
    out = e;
    return true;
}

bool validNameChars(const char* s, size_t len)
{
    if (len == 0 || len >= kAppNameChars) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        const char c = s[i];
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' ||
                        c == '-';
        if (!ok) {
            return false;
        }
    }
    return true;
}

}  // namespace

Sport sportFromCode(int64_t code)
{
    switch (code) {
        case 1: return Sport::Run;
        case 2: return Sport::Ride;
        case 3: return Sport::Walk;
        case 4: return Sport::Strength;
        case 5: return Sport::Workout;
        case 6: return Sport::Hybrid;
        case 7: return Sport::Row;
        default: return Sport::Other;
    }
}

bool valid(const Entry& e)
{
    return e.startUtc >= kMinStartUtc && e.tzOffsetMin >= kMinTzOffsetMin && e.tzOffsetMin <= kMaxTzOffsetMin &&
           e.durationSec <= kMaxDurationSec && e.distanceM <= kMaxDistanceM && e.avgHr <= e.maxHr;
}

int16_t tzOffsetMinutes(uint32_t utc, int64_t localSeconds)
{
    const int64_t diff = localSeconds - static_cast<int64_t>(utc);
    // Round half away from zero to a whole minute.
    const int64_t min = (diff >= 0 ? diff + 30 : diff - 30) / 60;
    if (min < kMinTzOffsetMin) {
        return kMinTzOffsetMin;
    }
    if (min > kMaxTzOffsetMin) {
        return kMaxTzOffsetMin;
    }
    return static_cast<int16_t>(min);
}

bool validAppName(const char* name)
{
    return name != nullptr && validNameChars(name, std::strlen(name));
}

bool decode(const char* text, size_t len, ActivityFile& out)
{
    if (text == nullptr || len == 0) {
        return false;
    }
    SDK::JsonStreamReader r(text, len);
    if (!r.validate()) {
        return false;
    }
    int64_t v = 0;
    if (!readInt(r, "v", kVersion, kVersion, v)) {
        return false;
    }
    std::string_view name;
    if (!r.get("app", name) || !validNameChars(name.data(), name.size())) {
        return false;
    }
    size_t n = 0;
    if (!r.getArrayLength("a", n) || n > kMaxEntries) {
        return false;
    }

    ActivityFile f;
    std::memcpy(f.app, name.data(), name.size());   // size checked above; f.app is zeroed
    for (unsigned i = 0; i < n; ++i) {
        if (readEntry(r, i, f.entries[f.count])) {
            ++f.count;
        } else {
            ++f.skipped;
        }
    }
    out = f;
    return true;
}

size_t encode(const ActivityFile& file, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    if (!validAppName(file.app) || file.count > kMaxEntries) {
        return 0;
    }
    for (unsigned i = 0; i < file.count; ++i) {
        if (!valid(file.entries[i])) {
            return 0;
        }
    }

    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", kVersion);
    w.add("app", static_cast<const char*>(file.app));
    w.startArray("a");
    for (unsigned i = 0; i < file.count; ++i) {
        const Entry& e = file.entries[i];
        w.startArray();
        w.add(e.startUtc);
        w.add(static_cast<uint16_t>(e.tzOffsetMin + kTzOffsetBias));
        w.add(e.durationSec);
        w.add(e.distanceM);
        w.add(e.avgHr);
        w.add(e.maxHr);
        w.add(static_cast<uint8_t>(e.sport));
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

size_t append(const char* oldText, size_t oldLen, const char* app, const Entry& entry, char* buf, size_t cap)
{
    if (buf != nullptr && cap > 0) {
        buf[0] = '\0';
    }
    if (!validAppName(app) || !valid(entry)) {
        return 0;
    }

    ActivityFile f;
    ActivityFile old;
    if (decode(oldText, oldLen, old) && std::strcmp(old.app, app) == 0) {
        f = old;
    }
    std::strncpy(f.app, app, kAppNameChars - 1);   // f.app is zeroed, so it stays terminated
    f.skipped = 0;

    // Find the place by start time, replacing an entry with the same start.
    uint8_t at = 0;
    while (at < f.count && f.entries[at].startUtc < entry.startUtc) {
        ++at;
    }
    if (at < f.count && f.entries[at].startUtc == entry.startUtc) {
        f.entries[at] = entry;
    } else {
        // A full file drops its oldest entry; an entry older than all of those is dropped itself.
        if (f.count == kMaxEntries) {
            if (at == 0) {
                return encode(f, buf, cap);
            }
            std::memmove(&f.entries[0], &f.entries[1], (at - 1) * sizeof(Entry));
            f.entries[at - 1] = entry;
        } else {
            std::memmove(&f.entries[at + 1], &f.entries[at], (f.count - at) * sizeof(Entry));
            f.entries[at] = entry;
            ++f.count;
        }
    }
    return encode(f, buf, cap);
}

}  // namespace HybridXLink
