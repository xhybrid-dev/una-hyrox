/**
 ******************************************************************************
 * @file    StateCodec.cpp
 * @brief   The streak's State as JSON (see the header).
 *
 * Layout (keys kept short: the file is read on every open):
 *   v     version
 *   g, pg goal and pending goal: [target, weekStart, scope, minMinutes, onePerDay]
 *   hp    has pending
 *   d     [installDay, period, weekStartDay, lastDay, trial]
 *   c     committed: [streak, weeks, longest, lifetime, shields, best, badges, lastWeek]
 *   p     pending decision: [missed, streak, sinceLastMiss]
 *   w     this week's flags: [overflow, celebrated, bestCelebrated]
 *   s     sessions: [[appKey, localStart, minutes, kind, app, flags], ...]
 *   r     ring: [next, floor];  k: [[appKey, localStart], ...] in slot order
 *   a     app names
 *   h     history: [next, [[count, target, outcome], ...]]
 ******************************************************************************
 */

#include "StateCodec.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include "SafeFile.hpp"

namespace Streak::StateCodec
{

namespace
{

void addGoal(SDK::JsonStreamWriter& w, const char* key, const Goal& g)
{
    w.startArray(key);
    w.add(g.target);
    w.add(g.weekStart);
    w.add(g.scope);
    w.add(g.minMinutes);
    w.add(static_cast<uint8_t>(g.onePerDay ? 1 : 0));
    w.endArray();
}

/// Bounded queries over one parsed document.
class Doc
{
public:
    Doc(const char* json, size_t len) : mReader(json, len) { mOk = mReader.validate(); }

    bool valid() const { return mOk; }

    template <typename T>
    bool get(T& out, const char* fmt, int a = 0, int b = 0)
    {
        char q[32];
        snprintf(q, sizeof(q), fmt, a, b);
        uint32_t v = 0;
        int32_t  s = 0;
        if (static_cast<T>(-1) < static_cast<T>(0)) {
            if (!mReader.get(q, s)) {
                return false;
            }
            out = static_cast<T>(s);
        } else {
            if (!mReader.get(q, v)) {
                return false;
            }
            out = static_cast<T>(v);
        }
        return true;
    }

    size_t length(const char* key)
    {
        size_t n = 0;
        return mReader.getArrayLength(key, n) ? n : 0;
    }

    bool text(char* out, size_t cap, const char* fmt, int a)
    {
        char q[32];
        snprintf(q, sizeof(q), fmt, a);
        const char* p   = nullptr;
        size_t      len = 0;
        if (!mReader.get(q, p, len)) {
            return false;
        }
        const size_t n = len < cap - 1 ? len : cap - 1;
        std::memcpy(out, p, n);
        out[n] = '\0';
        return true;
    }

private:
    SDK::JsonStreamReader mReader;
    bool                  mOk = false;
};

bool readGoal(Doc& d, const char* key, Goal& g)
{
    char    fmt[16];
    uint8_t one = 0;
    snprintf(fmt, sizeof(fmt), "%s[%%d]", key);
    const bool ok = d.get(g.target, fmt, 0) && d.get(g.weekStart, fmt, 1) && d.get(g.scope, fmt, 2)
                    && d.get(g.minMinutes, fmt, 3) && d.get(one, fmt, 4);
    g.onePerDay = one != 0;
    g           = g.sane();
    return ok;
}

} // namespace

size_t encode(const State& s, char* out, size_t cap)
{
    SDK::JsonStreamWriter w(out, cap);
    w.startMap();
    w.add("v", kVersion);
    addGoal(w, "g", s.goal);
    addGoal(w, "pg", s.pending);
    w.add("hp", static_cast<uint8_t>(s.hasPending ? 1 : 0));

    // Day numbers as their unsigned bit patterns: the SDK writer formats int32
    // with %ld, right on the watch's 32-bit long but not on a 64-bit host, and
    // "no period yet" is INT32_MIN.
    w.startArray("d");
    w.add(static_cast<uint32_t>(s.installDay));
    w.add(static_cast<uint32_t>(s.period));
    w.add(static_cast<uint32_t>(s.weekStartDay));
    w.add(static_cast<uint32_t>(s.lastDay));
    w.add(static_cast<uint8_t>(s.trial ? 1 : 0));
    w.endArray();

    w.startArray("c");
    w.add(s.streak);
    w.add(s.weeksAchieved);
    w.add(s.longest);
    w.add(s.lifetime);
    w.add(s.shields);
    w.add(s.bestWeek);
    w.add(s.badges);
    w.add(s.lastWeekCount);
    w.endArray();

    w.startArray("p");
    w.add(s.pendingMissed);
    w.add(s.pendingStreak);
    w.add(s.sinceLastMiss);
    w.endArray();

    w.startArray("w");
    w.add(s.overflow);
    w.add(static_cast<uint8_t>(s.celebrated ? 1 : 0));
    w.add(static_cast<uint8_t>(s.bestCelebrated ? 1 : 0));
    w.endArray();

    w.startArray("s");
    for (uint8_t i = 0; i < s.sessionCount && i < State::kMaxSessions; ++i) {
        const Session& x = s.sessions[i];
        w.startArray();
        w.add(x.appKey);
        w.add(x.localStart);
        w.add(x.minutes);
        w.add(static_cast<uint8_t>(x.kind));
        w.add(x.app);
        w.add(x.flags);
        w.endArray();
    }
    w.endArray();

    w.startArray("r");
    w.add(s.ringNext);
    w.add(s.ringFloor);
    w.endArray();
    w.startArray("k");
    for (uint8_t i = 0; i < s.ringCount && i < State::kRing; ++i) {
        w.startArray();
        w.add(s.ring[i].appKey);
        w.add(s.ring[i].localStart);
        w.endArray();
    }
    w.endArray();

    w.startArray("a");
    for (uint8_t i = 0; i < State::kApps; ++i) {
        w.add(s.apps[i]);
    }
    w.endArray();

    w.startArray("h");
    w.add(s.historyNext);
    w.startArray();
    for (uint8_t i = 0; i < s.historyCount && i < State::kHistory; ++i) {
        const WeekRecord& r = s.history[i];
        w.startArray();
        w.add(r.count);
        w.add(r.target);
        w.add(static_cast<uint8_t>(r.outcome));
        w.endArray();
    }
    w.endArray();
    w.endArray();

    w.endMap();
    if (w.isError()) {
        return 0;
    }
    return std::strlen(out);
}

bool decode(const char* json, size_t len, State& out)
{
    Doc d(json, len);
    if (!d.valid()) {
        return false;
    }
    uint16_t version = 0;
    if (!d.get(version, "v") || version != kVersion) {
        return false;
    }

    State   s {};
    uint8_t flag = 0;
    bool    ok   = readGoal(d, "g", s.goal) && readGoal(d, "pg", s.pending) && d.get(flag, "hp");
    s.hasPending = flag != 0;

    uint32_t days[4] = {};
    ok = ok && d.get(days[0], "d[0]") && d.get(days[1], "d[1]") && d.get(days[2], "d[2]") && d.get(days[3], "d[3]")
         && d.get(flag, "d[4]");
    s.installDay   = static_cast<int32_t>(days[0]);
    s.period       = static_cast<int32_t>(days[1]);
    s.weekStartDay = static_cast<int32_t>(days[2]);
    s.lastDay      = static_cast<int32_t>(days[3]);
    s.trial = flag != 0;

    ok = ok && d.get(s.streak, "c[0]") && d.get(s.weeksAchieved, "c[1]") && d.get(s.longest, "c[2]")
         && d.get(s.lifetime, "c[3]") && d.get(s.shields, "c[4]") && d.get(s.bestWeek, "c[5]")
         && d.get(s.badges, "c[6]") && d.get(s.lastWeekCount, "c[7]");
    ok = ok && d.get(s.pendingMissed, "p[0]") && d.get(s.pendingStreak, "p[1]") && d.get(s.sinceLastMiss, "p[2]");
    uint8_t celebrated = 0, best = 0;
    ok = ok && d.get(s.overflow, "w[0]") && d.get(celebrated, "w[1]") && d.get(best, "w[2]");
    s.celebrated     = celebrated != 0;
    s.bestCelebrated = best != 0;
    if (!ok) {
        return false;
    }
    s.shields = s.shields > 2 ? 2 : s.shields;

    // This week's sessions.
    const size_t sessions = d.length("s");
    s.sessionCount        = static_cast<uint8_t>(sessions < State::kMaxSessions ? sessions : State::kMaxSessions);
    for (uint8_t i = 0; i < s.sessionCount; ++i) {
        Session& x    = s.sessions[i];
        uint8_t  kind = 0;
        if (!(d.get(x.appKey, "s[%d][0]", i) && d.get(x.localStart, "s[%d][1]", i) && d.get(x.minutes, "s[%d][2]", i)
              && d.get(kind, "s[%d][3]", i) && d.get(x.app, "s[%d][4]", i) && d.get(x.flags, "s[%d][5]", i))) {
            return false;
        }
        x.kind = kind < kKindCount ? static_cast<Kind>(kind) : Kind::Other;
        if (x.app >= State::kApps && x.app != kManualApp) {
            x.app = kNoApp;
        }
    }

    // The dedup ring.
    ok = d.get(s.ringNext, "r[0]") && d.get(s.ringFloor, "r[1]");
    if (!ok) {
        return false;
    }
    const size_t keys = d.length("k");
    s.ringCount       = static_cast<uint8_t>(keys < State::kRing ? keys : State::kRing);
    s.ringNext        = static_cast<uint8_t>(s.ringNext % State::kRing);
    for (uint8_t i = 0; i < s.ringCount; ++i) {
        if (!(d.get(s.ring[i].appKey, "k[%d][0]", i) && d.get(s.ring[i].localStart, "k[%d][1]", i))) {
            return false;
        }
    }

    for (uint8_t i = 0; i < State::kApps; ++i) {
        d.text(s.apps[i], kAppNameChars, "a[%d]", i);
    }

    // History.
    ok = d.get(s.historyNext, "h[0]");
    if (!ok) {
        return false;
    }
    s.historyNext       = static_cast<uint8_t>(s.historyNext % State::kHistory);
    const size_t weeks  = d.length("h[1]");
    s.historyCount      = static_cast<uint8_t>(weeks < State::kHistory ? weeks : State::kHistory);
    for (uint8_t i = 0; i < s.historyCount; ++i) {
        uint8_t outcome = 0;
        if (!(d.get(s.history[i].count, "h[1][%d][0]", i) && d.get(s.history[i].target, "h[1][%d][1]", i)
              && d.get(outcome, "h[1][%d][2]", i))) {
            return false;
        }
        s.history[i].outcome = outcome <= static_cast<uint8_t>(Outcome::Void) ? static_cast<Outcome>(outcome)
                                                                                : Outcome::Void;
    }

    out = s;
    return true;
}

bool save(SDK::Interface::IFileSystem& fs, const char* path, const State& s, char* scratch, size_t cap)
{
    const size_t len = encode(s, scratch, cap);
    return len > 0 && SafeFile::write(fs, path, scratch, len);
}

Source load(SDK::Interface::IFileSystem& fs, const char* path, State& s, char* scratch, size_t cap)
{
    for (const bool backup : {false, true}) {
        const size_t len = SafeFile::read(fs, path, backup, scratch, cap);
        if (len > 0 && decode(scratch, len, s)) {
            return backup ? Source::Backup : Source::Primary;
        }
    }
    return Source::None;
}

} // namespace Streak::StateCodec
