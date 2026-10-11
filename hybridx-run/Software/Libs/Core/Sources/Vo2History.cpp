/**
 ******************************************************************************
 * @file    Vo2History.cpp
 * @brief   The VO2max history ring and its JSON.
 ******************************************************************************
 */

#include "Vo2History.hpp"

#include <cstdio>
#include <cstring>

namespace RunVo2
{

namespace
{

/// A bounded cursor over the JSON text.
struct Reader {
    const char* p;
    const char* end;

    void skipSpace()
    {
        while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) {
            ++p;
        }
    }
    bool take(char c)
    {
        skipSpace();
        if (p < end && *p == c) {
            ++p;
            return true;
        }
        return false;
    }
    bool peek(char c)
    {
        skipSpace();
        return p < end && *p == c;
    }
    /// A non-negative integer no larger than max.
    bool number(uint32_t max, uint32_t& out)
    {
        skipSpace();
        if (p >= end || *p < '0' || *p > '9') {
            return false;
        }
        uint64_t v = 0;
        while (p < end && *p >= '0' && *p <= '9') {
            v = v * 10 + static_cast<uint64_t>(*p - '0');
            if (v > max) {
                return false;
            }
            ++p;
        }
        out = static_cast<uint32_t>(v);
        return true;
    }
    /// A key without escapes, into key (truncated keys fail to match later).
    bool key(char* out, size_t cap)
    {
        if (!take('"')) {
            return false;
        }
        size_t n = 0;
        while (p < end && *p != '"') {
            if (*p == '\\') {
                return false;
            }
            if (n + 1 < cap) {
                out[n++] = *p;
            }
            ++p;
        }
        out[n] = '\0';
        return take('"') && take(':');
    }
};

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
}

const RunRecord& Vo2History::at(uint8_t i) const
{
    static const RunRecord kNone{};
    if (i >= mCount) {
        return kNone;
    }
    return mRuns[(mFirst + i) % Config::kHistoryRuns];
}

uint16_t Vo2History::rollingX10() const
{
    uint32_t sum = 0;
    uint32_t weight = 0;
    const uint8_t from = mCount > Config::kRollingRuns ? mCount - Config::kRollingRuns : 0;
    for (uint8_t i = from; i < mCount; ++i) {
        const RunRecord& r = at(i);
        const uint32_t w = r.windows < Config::kMaxRunWeight ? r.windows : Config::kMaxRunWeight;
        sum    += static_cast<uint32_t>(r.vo2x10) * w;
        weight += w;
    }
    if (weight == 0) {
        return 0;
    }
    return static_cast<uint16_t>((sum + weight / 2) / weight);
}

void Vo2History::raiseAutoMaxHr(uint8_t bpm)
{
    if (bpm > mAutoMaxHr && bpm <= Config::kMaxHrBpm) {
        mAutoMaxHr = bpm;
    }
}

size_t Vo2History::toJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    size_t n = 0;
    auto put = [&](int w) {
        if (w < 0 || static_cast<size_t>(w) >= cap - n) {
            n = cap;   // mark overflow
            return false;
        }
        n += static_cast<size_t>(w);
        return true;
    };
    if (!put(std::snprintf(buf, cap, "{\"v\":1,\"autoMaxHr\":%u,\"runs\":[",
                           static_cast<unsigned>(mAutoMaxHr)))) {
        buf[0] = '\0';
        return 0;
    }
    for (uint8_t i = 0; i < mCount; ++i) {
        const RunRecord& r = at(i);
        if (!put(std::snprintf(buf + n, cap - n, "%s[%lu,%u,%u]", i ? "," : "",
                               static_cast<unsigned long>(r.utc),
                               static_cast<unsigned>(r.vo2x10),
                               static_cast<unsigned>(r.windows)))) {
            buf[0] = '\0';
            return 0;
        }
    }
    if (!put(std::snprintf(buf + n, cap - n, "]}"))) {
        buf[0] = '\0';
        return 0;
    }
    return n;
}

bool Vo2History::fromJson(const char* text, size_t len)
{
    clear();
    if (text == nullptr) {
        return false;
    }
    Reader rd{text, text + len};
    Vo2History h;
    bool sawVersion = false;

    if (!rd.take('{')) {
        return false;
    }
    if (!rd.peek('}')) {
        do {
            char k[16];
            if (!rd.key(k, sizeof(k))) {
                return false;
            }
            uint32_t v = 0;
            if (std::strcmp(k, "v") == 0) {
                if (!rd.number(1000, v) || v != 1) {
                    return false;
                }
                sawVersion = true;
            } else if (std::strcmp(k, "autoMaxHr") == 0) {
                if (!rd.number(Config::kMaxHrBpm, v)) {
                    return false;
                }
                h.mAutoMaxHr = static_cast<uint8_t>(v);
            } else if (std::strcmp(k, "runs") == 0) {
                if (!rd.take('[')) {
                    return false;
                }
                if (!rd.peek(']')) {
                    do {
                        uint32_t utc = 0, vo2 = 0, win = 0;
                        if (!rd.take('[') || !rd.number(0xFFFFFFFFu, utc) || !rd.take(',') ||
                            !rd.number(static_cast<uint32_t>(Config::kMaxPlausibleVo2 * 10.0f), vo2) || !rd.take(',') ||
                            !rd.number(Config::kMaxWindows, win) || !rd.take(']')) {
                            return false;
                        }
                        h.add(RunRecord{utc, static_cast<uint16_t>(vo2), static_cast<uint16_t>(win)});
                    } while (rd.take(','));
                }
                if (!rd.take(']')) {
                    return false;
                }
            } else {
                return false;   // unknown keys: not a file we wrote
            }
        } while (rd.take(','));
    }
    if (!rd.take('}') || !sawVersion) {
        return false;
    }
    *this = h;
    return true;
}

size_t Vo2History::toSharedJson(char* buf, size_t cap) const
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    const uint32_t last = mCount ? at(static_cast<uint8_t>(mCount - 1)).utc : 0;
    const int w = std::snprintf(buf, cap, "{\"v\":1,\"vo2maxX10\":%u,\"runs\":%u,\"utc\":%lu}",
                                static_cast<unsigned>(rollingX10()),
                                static_cast<unsigned>(mCount),
                                static_cast<unsigned long>(last));
    if (w < 0 || static_cast<size_t>(w) >= cap) {
        buf[0] = '\0';
        return 0;
    }
    return static_cast<size_t>(w);
}

}  // namespace RunVo2
