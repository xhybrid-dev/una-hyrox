/**
 ******************************************************************************
 * @file    StateFile.cpp
 * @brief   The Energy Probe's state.txt (see the header).
 ******************************************************************************
 */

#include "StateFile.hpp"

#include <cstring>

namespace EnergyProbe
{

namespace
{

constexpr int64_t kVersion = 1;

/// A bounded writer of `key=value` lines; integers by hand (no 64-bit printf on the watch).
class Writer
{
public:
    Writer(char* buf, size_t cap) : mBuf(buf), mCap(cap) {}

    void raw(const char* s)
    {
        const size_t n = std::strlen(s);
        if (!mOk || mLen + n + 1 > mCap) {
            mOk = false;
            return;
        }
        std::memcpy(mBuf + mLen, s, n);
        mLen += n;
        mBuf[mLen] = '\0';
    }
    void num(int64_t v)
    {
        char t[24];
        char* p = t + sizeof(t) - 1;
        *p = '\0';
        const bool neg = v < 0;
        uint64_t m = neg ? static_cast<uint64_t>(-(v + 1)) + 1u : static_cast<uint64_t>(v);
        do {
            *--p = static_cast<char>('0' + m % 10u);
            m /= 10u;
        } while (m != 0 && p > t + 1);
        if (neg) {
            *--p = '-';
        }
        raw(p);
    }
    void kv(const char* key, int64_t v)
    {
        raw(key);
        raw("=");
        num(v);
        raw("\n");
    }
    bool   ok() const { return mOk; }
    size_t len() const { return mLen; }

private:
    char*  mBuf;
    size_t mCap;
    size_t mLen = 0;
    bool   mOk  = true;
};

/// Parses a signed integer at `p` (up to `end`); advances `p`. False if there is none.
bool parseInt(const char*& p, const char* end, int64_t& out)
{
    bool neg = false;
    if (p < end && (*p == '-' || *p == '+')) {
        neg = *p == '-';
        ++p;
    }
    if (p >= end || *p < '0' || *p > '9') {
        return false;
    }
    int64_t v = 0;
    int digits = 0;
    while (p < end && *p >= '0' && *p <= '9') {
        if (++digits > 18) {
            return false;
        }
        v = v * 10 + (*p - '0');
        ++p;
    }
    out = neg ? -v : v;
    return true;
}

bool fitsI32(int64_t v) { return v >= INT32_MIN && v <= INT32_MAX; }
bool fitsU32(int64_t v) { return v >= 0 && v <= 0xFFFFFFFFll; }

} // namespace

void pushHistory(ProbeState& s, const NightBrief& b)
{
    const uint8_t keep = s.historyCount < kHistory ? s.historyCount : static_cast<uint8_t>(kHistory - 1);
    for (int i = keep; i > 0; --i) {
        s.history[i] = s.history[i - 1];
    }
    s.history[0] = b;
    if (s.historyCount < kHistory) {
        ++s.historyCount;
    }
}

size_t writeState(char* out, size_t cap, const ProbeState& s)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    out[0] = '\0';
    Writer w(out, cap);
    w.kv("v", kVersion);
    w.kv("window", s.settings.window);
    w.kv("day_mode", static_cast<int64_t>(s.settings.dayMode));
    w.kv("first", s.settings.firstNight);
    w.kv("ovr_day", s.settings.overrideDay);
    w.kv("ovr_mode", s.settings.overrideMode);
    w.kv("starts", s.starts);

    const NightOpen& n = s.night;
    w.kv("n_open", n.open ? 1 : 0);
    if (n.open) {
        w.kv("n_day", n.day);
        w.kv("n_mode", n.mode);
        w.kv("n_flags", n.flags);
        w.kv("n_start", n.startUtc);
        w.kv("n_pct", n.startPct);
        w.kv("n_mah", n.startMah);
        w.kv("n_rows", n.rows);
        w.kv("n_worn", n.wornMin);
        w.kv("n_hr", n.hrMin);
        w.kv("n_gsum", n.gaugeSum);
        w.kv("n_gn", n.gaugeN);
        w.kv("n_lutc", n.lastUtc);
        w.kv("n_lpct", n.lastPct);
        w.kv("n_lmah", n.lastMah);
    }
    for (uint8_t i = 0; i < s.historyCount && i < kHistory; ++i) {
        const NightBrief& b = s.history[i];
        char key[4] = {'h', static_cast<char>('0' + i), '\0', '\0'};
        w.raw(key);
        w.raw("=");
        w.num(b.day);
        w.raw(",");
        w.num(b.mode);
        w.raw(",");
        w.num(b.flags);
        w.raw(",");
        w.num(b.drainX100);
        w.raw(",");
        w.num(b.capMaX100);
        w.raw(",");
        w.num(b.gaugeMaX100);
        w.raw(",");
        w.num(b.hoursX100);
        w.raw("\n");
    }
    if (!w.ok()) {
        out[0] = '\0';
        return 0;
    }
    return w.len();
}

bool readState(const char* text, size_t len, ProbeState& out)
{
    out = ProbeState{};
    if (text == nullptr) {
        return false;
    }
    ProbeState s;
    bool versioned = false;
    const char* p   = text;
    const char* end = text + len;
    while (p < end) {
        const char* eol = p;
        while (eol < end && *eol != '\n' && *eol != '\0') {
            ++eol;
        }
        const char* eq = p;
        while (eq < eol && *eq != '=') {
            ++eq;
        }
        if (eq < eol) {
            const size_t klen = static_cast<size_t>(eq - p);
            char key[12] = {};
            if (klen > 0 && klen < sizeof(key)) {
                std::memcpy(key, p, klen);
                const char* v = eq + 1;
                int64_t x = 0;
                if (key[0] == 'h' && klen == 2 && key[1] >= '0' && key[1] <= '9') {
                    int64_t f[7] = {};
                    bool good = true;
                    for (int k = 0; k < 7 && good; ++k) {
                        good = parseInt(v, eol, f[k]) && fitsI32(f[k]);
                        if (good && k < 6) {
                            good = v < eol && *v == ',';
                            ++v;
                        }
                    }
                    const uint8_t idx = static_cast<uint8_t>(key[1] - '0');
                    if (good && idx == s.historyCount && idx < kHistory && f[1] >= 0 && f[1] < kModeCount &&
                        f[2] >= 0 && f[2] < 256) {
                        NightBrief& b = s.history[idx];
                        b.day = static_cast<int32_t>(f[0]);
                        b.mode = static_cast<uint8_t>(f[1]);
                        b.flags = static_cast<uint8_t>(f[2]);
                        b.drainX100 = static_cast<int32_t>(f[3]);
                        b.capMaX100 = static_cast<int32_t>(f[4]);
                        b.gaugeMaX100 = static_cast<int32_t>(f[5]);
                        b.hoursX100 = static_cast<int32_t>(f[6]);
                        ++s.historyCount;
                    }
                } else if (parseInt(v, eol, x)) {
                    if (std::strcmp(key, "v") == 0) {
                        versioned = x == kVersion;
                    } else if (std::strcmp(key, "window") == 0 && x >= 0 && x < kWindowCount) {
                        s.settings.window = static_cast<uint8_t>(x);
                    } else if (std::strcmp(key, "day_mode") == 0 && (x == 0 || x == 1)) {
                        s.settings.dayMode = modeFrom(static_cast<uint32_t>(x));
                    } else if (std::strcmp(key, "first") == 0 && fitsI32(x)) {
                        s.settings.firstNight = static_cast<int32_t>(x);
                    } else if (std::strcmp(key, "ovr_day") == 0 && fitsI32(x)) {
                        s.settings.overrideDay = static_cast<int32_t>(x);
                    } else if (std::strcmp(key, "ovr_mode") == 0 && x >= 0 && x < kOverrideCount) {
                        s.settings.overrideMode = static_cast<uint8_t>(x);
                    } else if (std::strcmp(key, "starts") == 0 && fitsU32(x)) {
                        s.starts = static_cast<uint32_t>(x);
                    } else if (std::strcmp(key, "n_open") == 0) {
                        s.night.open = x == 1;
                    } else if (std::strcmp(key, "n_day") == 0 && fitsI32(x)) {
                        s.night.day = static_cast<int32_t>(x);
                    } else if (std::strcmp(key, "n_mode") == 0 && x >= 0 && x < kModeCount) {
                        s.night.mode = static_cast<uint8_t>(x);
                    } else if (std::strcmp(key, "n_flags") == 0 && x >= 0 && x < 256) {
                        s.night.flags = static_cast<uint8_t>(x);
                    } else if (std::strcmp(key, "n_start") == 0 && fitsU32(x)) {
                        s.night.startUtc = static_cast<uint32_t>(x);
                    } else if (std::strcmp(key, "n_pct") == 0 && fitsI32(x)) {
                        s.night.startPct = static_cast<int32_t>(x);
                    } else if (std::strcmp(key, "n_mah") == 0 && fitsI32(x)) {
                        s.night.startMah = static_cast<int32_t>(x);
                    } else if (std::strcmp(key, "n_rows") == 0 && fitsU32(x)) {
                        s.night.rows = static_cast<uint32_t>(x);
                    } else if (std::strcmp(key, "n_worn") == 0 && fitsU32(x)) {
                        s.night.wornMin = static_cast<uint32_t>(x);
                    } else if (std::strcmp(key, "n_hr") == 0 && fitsU32(x)) {
                        s.night.hrMin = static_cast<uint32_t>(x);
                    } else if (std::strcmp(key, "n_gsum") == 0) {
                        s.night.gaugeSum = x;
                    } else if (std::strcmp(key, "n_gn") == 0 && fitsU32(x)) {
                        s.night.gaugeN = static_cast<uint32_t>(x);
                    } else if (std::strcmp(key, "n_lutc") == 0 && fitsU32(x)) {
                        s.night.lastUtc = static_cast<uint32_t>(x);
                    } else if (std::strcmp(key, "n_lpct") == 0 && fitsI32(x)) {
                        s.night.lastPct = static_cast<int32_t>(x);
                    } else if (std::strcmp(key, "n_lmah") == 0 && fitsI32(x)) {
                        s.night.lastMah = static_cast<int32_t>(x);
                    }
                }
            }
        }
        p = eol < end ? eol + 1 : end;
    }
    if (!versioned) {
        return false;
    }
    out = s;
    return true;
}

} // namespace EnergyProbe
