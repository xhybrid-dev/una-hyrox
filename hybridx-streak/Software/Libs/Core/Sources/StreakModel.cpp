/**
 ******************************************************************************
 * @file    StreakModel.cpp
 * @brief   The streak's rules (see the header).
 ******************************************************************************
 */

#include "StreakModel.hpp"

#include <cstring>

#include "Summits.hpp"
#include "WeekMath.hpp"

namespace Streak
{

namespace
{
constexpr int32_t  kSecondsDay       = 86400;
constexpr uint8_t  kShieldEvery      = 4;     ///< S3: one shield per 4 achieved weeks
constexpr uint8_t  kMaxShields       = 2;     ///< S3
constexpr uint8_t  kBadgeCount       = sizeof(kSessionBadges) / sizeof(kSessionBadges[0]);

template <typename T>
T maxOf(T a, T b)
{
    return a > b ? a : b;
}

/// Did going from `before` to `after` weeks cross a summit? Sets the climb reached.
bool summitBetween(uint32_t before, uint32_t after, uint8_t& climb)
{
    if (after <= before) {
        return false;
    }
    const ClimbPosition a = climbFor(before);
    const ClimbPosition b = climbFor(after);
    if (a.climb != b.climb || a.ascent != b.ascent) {
        climb = a.climb;
        return true;
    }
    return false;
}
} // namespace

// -- Setup ------------------------------------------------------------------------------

void StreakModel::reset(const Goal& goal)
{
    mState      = State {};
    mState.goal = goal.sane();
}

void StreakModel::startWeek(int32_t period, int32_t startDay, bool trial)
{
    mState.period         = period;
    mState.weekStartDay   = startDay;
    mState.trial          = trial;
    mState.sessionCount   = 0;
    mState.overflow       = 0;
    mState.celebrated     = false;
    mState.bestCelebrated = false;
}

void StreakModel::scanWindow(int32_t nowDay, int32_t& fromDay, int32_t& toDay) const
{
    const uint8_t ws = mState.goal.weekStart;
    toDay            = nowDay;
    if (mState.period == State::kNoPeriod) {
        // First open: this week only; nothing from before install (S15).
        fromDay = WeekMath::periodStartDay(WeekMath::periodOf(nowDay, ws), ws);
        return;
    }
    const int32_t effNow   = maxOf(nowDay, mState.lastDay);
    const int32_t prevWeek = WeekMath::periodStartDay(WeekMath::periodOf(effNow, ws), ws) - 7;
    fromDay                = mState.weekStartDay < prevWeek ? mState.weekStartDay : prevWeek;
    fromDay                = maxOf(fromDay, nowDay - kMaxCatchUpDays);
    const int32_t installWeek = WeekMath::periodStartDay(WeekMath::periodOf(mState.installDay, ws), ws);
    fromDay                   = maxOf(fromDay, installWeek);
}

// -- Dedup -------------------------------------------------------------------------------

bool StreakModel::seen(uint32_t appKey, uint32_t localStart) const
{
    if (mState.ringFloor != 0 && localStart <= mState.ringFloor) {
        return true;
    }
    for (uint8_t i = 0; i < mState.ringCount; ++i) {
        if (mState.ring[i].appKey == appKey && mState.ring[i].localStart == localStart) {
            return true;
        }
    }
    return false;
}

void StreakModel::remember(uint32_t appKey, uint32_t localStart)
{
    if (mState.ringCount == State::kRing) {
        // Evicting: anything this old is now treated as seen (PLAN 5.5).
        mState.ringFloor = maxOf(mState.ringFloor, mState.ring[mState.ringNext].localStart);
    }
    mState.ring[mState.ringNext] = Key { appKey, localStart };
    mState.ringNext              = static_cast<uint8_t>((mState.ringNext + 1) % State::kRing);
    if (mState.ringCount < State::kRing) {
        ++mState.ringCount;
    }
}

uint8_t StreakModel::appIndex(const char* name)
{
    for (uint8_t i = 0; i < State::kApps; ++i) {
        if (mState.apps[i][0] != '\0' && std::strncmp(mState.apps[i], name, kAppNameChars - 1) == 0) {
            return i;
        }
    }
    for (uint8_t pass = 0; pass < 2; ++pass) {
        for (uint8_t i = 0; i < State::kApps; ++i) {
            bool free = mState.apps[i][0] == '\0';
            if (!free && pass == 1) {
                // Reuse a slot no session this week refers to.
                free = true;
                for (uint8_t s = 0; s < mState.sessionCount; ++s) {
                    free = free && mState.sessions[s].app != i;
                }
            }
            if (free) {
                std::strncpy(mState.apps[i], name, kAppNameChars - 1);
                mState.apps[i][kAppNameChars - 1] = '\0';
                return i;
            }
        }
    }
    return kNoApp;
}

// -- Status ---------------------------------------------------------------------------------

Status StreakModel::status(uint8_t index) const
{
    if (index >= mState.sessionCount) {
        return Status::Excluded;
    }
    const Session& s = mState.sessions[index];
    const Goal&    g = mState.goal;
    if (s.flags & Session::kExcluded) {
        return Status::Excluded;
    }
    if (g.scope != kScopeAny && static_cast<uint8_t>(s.kind) != g.scope) {
        return Status::OutOfScope;
    }
    if (!(s.flags & Session::kManual) && g.minMinutes > 0 && s.minutes < g.minMinutes) {
        return Status::TooShort;
    }
    if (g.onePerDay) {
        const int32_t day = static_cast<int32_t>(s.localStart / kSecondsDay);
        for (uint8_t j = 0; j < index; ++j) {
            if (static_cast<int32_t>(mState.sessions[j].localStart / kSecondsDay) == day
                && status(j) == Status::Counts) {
                return Status::SameDay;
            }
        }
    }
    return Status::Counts;
}

uint8_t StreakModel::qualifying() const
{
    uint16_t n = mState.overflow;
    for (uint8_t i = 0; i < mState.sessionCount; ++i) {
        n = static_cast<uint16_t>(n + (status(i) == Status::Counts ? 1 : 0));
    }
    return static_cast<uint8_t>(n > 0xFF ? 0xFF : n);
}

uint16_t StreakModel::liveStreak() const
{
    const uint16_t base = mState.pendingMissed ? static_cast<uint16_t>(mState.pendingStreak + mState.sinceLastMiss)
                                               : mState.streak;
    return static_cast<uint16_t>(base + (weekMet() ? 1 : 0));
}

uint16_t StreakModel::liveWeeks() const
{
    return static_cast<uint16_t>(mState.weeksAchieved + (weekMet() ? 1 : 0));
}

uint16_t StreakModel::liveLifetime() const
{
    return static_cast<uint16_t>(mState.lifetime + qualifying());
}

// -- Crediting ---------------------------------------------------------------------------------

void StreakModel::credit(const Found& f, Events& ev)
{
    remember(f.appKey, f.localStart);

    Session s;
    s.appKey     = f.appKey;
    s.localStart = f.localStart;
    s.minutes    = f.minutes;
    s.kind       = f.kind;
    s.app        = appIndex(f.app);

    if (mState.sessionCount >= State::kMaxSessions) {
        // Full: count it if it would count, without listing it.
        const Goal& g = mState.goal;
        const bool  ok = (g.scope == kScopeAny || static_cast<uint8_t>(s.kind) == g.scope)
                        && (g.minMinutes == 0 || s.minutes >= g.minMinutes);
        if (ok && mState.overflow < 0xFF) {
            ++mState.overflow;
            ev.add(EventKind::SessionFound, static_cast<uint8_t>(s.kind), s.minutes);
        }
        return;
    }
    // Keep the list in start order.
    uint8_t at = mState.sessionCount;
    while (at > 0 && mState.sessions[at - 1].localStart > s.localStart) {
        mState.sessions[at] = mState.sessions[at - 1];
        --at;
    }
    mState.sessions[at] = s;
    ++mState.sessionCount;
    if (status(at) == Status::Counts) {
        ev.add(EventKind::SessionFound, static_cast<uint8_t>(s.kind), s.minutes);
    }
}

void StreakModel::update(int32_t nowDay, const Found* found, size_t count, Events& ev)
{
    const Goal& g = mState.goal;
    if (mState.period == State::kNoPeriod) {
        const int32_t p = WeekMath::periodOf(nowDay, g.weekStart);
        startWeek(p, WeekMath::periodStartDay(p, g.weekStart), true);   // S9, S15: a trial first week
        mState.installDay = nowDay;
        mState.lastDay    = nowDay;
    }
    // Never roll back: a clock set backwards leaves the weeks where they are.
    const int32_t effNow = maxOf(nowDay, mState.lastDay);
    mState.lastDay       = effNow;

    Events  pastEvents;   // toasts for weeks that close now are not played
    size_t  i = 0;
    while (true) {
        const int32_t weekEnd = mState.weekStartDay + 7;   // exclusive
        const bool    current = effNow < weekEnd;
        while (i < count && found[i].localDay < weekEnd) {
            const Found& f = found[i++];
            if (seen(f.appKey, f.localStart)) {
                continue;
            }
            if (f.localDay < mState.weekStartDay) {
                // Its week was judged already: it joins the lifetime total only
                // (PLAN 6.2: committed weeks are final).
                remember(f.appKey, f.localStart);
                const bool ok = (g.scope == kScopeAny || static_cast<uint8_t>(f.kind) == g.scope)
                                && (g.minMinutes == 0 || f.minutes >= g.minMinutes);
                if (ok && mState.lifetime < 0xFFFF) {
                    ++mState.lifetime;
                }
                continue;
            }
            credit(f, current ? ev : pastEvents);
        }
        if (current) {
            break;
        }
        closeWeek(ev, effNow < weekEnd + 7);

        // The next week, under any goal that was waiting for it.
        bool trial = false;
        if (mState.hasPending) {
            trial = mState.pending.target != mState.goal.target || mState.pending.scope != mState.goal.scope;
            const uint8_t ws  = mState.goal.weekStart;   // start-day changes never wait (setGoal)
            mState.goal       = mState.pending;
            mState.goal.weekStart = ws;
            mState.hasPending = false;
        }
        startWeek(mState.period + 1, weekEnd, trial);
    }
    // Anything dated after "now" (a clock set back) is left for a later scan.

    // A decision the catch-up needs.
    if (mState.pendingMissed > 0) {
        if (mState.shields >= mState.pendingMissed) {
            ev.add(EventKind::ShieldOffer, mState.pendingMissed,
                   static_cast<uint16_t>(mState.pendingStreak + mState.sinceLastMiss));
        } else {
            ev.add(EventKind::StreakReset, 0, static_cast<uint16_t>(mState.pendingStreak + mState.sinceLastMiss));
            mState.streak        = mState.sinceLastMiss;
            mState.pendingMissed = 0;
            mState.pendingStreak = 0;
            mState.sinceLastMiss = 0;
        }
    }
    afterChange(ev);
}

// -- Closing weeks ---------------------------------------------------------------------------

void StreakModel::addHistory(uint8_t count, uint8_t target, Outcome outcome)
{
    mState.history[mState.historyNext] = WeekRecord { count, target, outcome };
    mState.historyNext                 = static_cast<uint8_t>((mState.historyNext + 1) % State::kHistory);
    if (mState.historyCount < State::kHistory) {
        ++mState.historyCount;
    }
}

void StreakModel::commitAchieved(uint8_t count, Events& ev)
{
    const uint16_t before = mState.weeksAchieved;
    ++mState.weeksAchieved;
    if (mState.pendingMissed > 0) {
        ++mState.sinceLastMiss;
    } else {
        ++mState.streak;
        mState.longest = maxOf(mState.longest, mState.streak);
    }
    if (!mState.celebrated) {
        // Achieved without being seen live (found in a catch-up): step up now.
        ev.add(EventKind::StepUp, 0, mState.weeksAchieved);
        uint8_t climb = 0;
        if (summitBetween(before, mState.weeksAchieved, climb)) {
            ev.add(EventKind::Summit, climb, mState.weeksAchieved);
        }
    }
    if (mState.weeksAchieved % kShieldEvery == 0 && mState.shields < kMaxShields) {
        ++mState.shields;
        ev.add(EventKind::ShieldEarned, mState.shields);
    }
    (void)count;
}

void StreakModel::closeWeek(Events& ev, bool isLast)
{
    const uint8_t q      = qualifying();
    const uint8_t target = mState.goal.target;
    Outcome       outcome;

    mState.lifetime = static_cast<uint16_t>(mState.lifetime + q > 0xFFFF ? 0xFFFF : mState.lifetime + q);
    if (q >= target) {
        commitAchieved(q, ev);
        outcome = Outcome::Achieved;
    } else if (mState.trial) {
        outcome = Outcome::Trial;   // S9: counts if met, can't break the streak
    } else {
        outcome             = Outcome::Missed;
        const uint16_t base = mState.pendingMissed ? static_cast<uint16_t>(mState.pendingStreak + mState.sinceLastMiss)
                                                   : mState.streak;
        if (base > 0) {
            if (mState.pendingMissed == 0) {
                mState.pendingStreak = mState.streak;
                mState.streak        = 0;
            } else {
                mState.pendingStreak = static_cast<uint16_t>(mState.pendingStreak + mState.sinceLastMiss);
            }
            mState.sinceLastMiss = 0;
            ++mState.pendingMissed;
        }
    }
    mState.bestWeek      = maxOf(mState.bestWeek, q);
    mState.lastWeekCount = q;
    addHistory(q, target, outcome);
    if (isLast) {
        ev.add(EventKind::WeekResult, q, static_cast<uint16_t>(target | (static_cast<uint16_t>(outcome) << 8)));
    }
}

void StreakModel::decideShields(bool use, Events& ev)
{
    if (mState.pendingMissed == 0) {
        return;
    }
    if (use && mState.shields >= mState.pendingMissed) {
        mState.shields = static_cast<uint8_t>(mState.shields - mState.pendingMissed);
        mState.streak  = static_cast<uint16_t>(mState.pendingStreak + mState.sinceLastMiss);
        mState.longest = maxOf(mState.longest, mState.streak);
        // The missed weeks are now shielded ones.
        uint8_t left = mState.pendingMissed;
        for (uint8_t k = 1; k <= mState.historyCount && left > 0; ++k) {
            WeekRecord& r = mState.history[(mState.historyNext + State::kHistory - k) % State::kHistory];
            if (r.outcome == Outcome::Missed) {
                r.outcome = Outcome::Shielded;
                --left;
            }
        }
    } else {
        ev.add(EventKind::StreakReset, 0, static_cast<uint16_t>(mState.pendingStreak + mState.sinceLastMiss));
        mState.streak = mState.sinceLastMiss;
    }
    mState.pendingMissed = 0;
    mState.pendingStreak = 0;
    mState.sinceLastMiss = 0;
    afterChange(ev);
}

// -- The live week -------------------------------------------------------------------------------

void StreakModel::afterChange(Events& ev)
{
    const bool met = weekMet();
    if (met && !mState.celebrated) {
        mState.celebrated = true;
        ev.add(EventKind::StepUp, 0, liveWeeks());
        uint8_t climb = 0;
        if (summitBetween(mState.weeksAchieved, liveWeeks(), climb)) {
            ev.add(EventKind::Summit, climb, liveWeeks());
        }
    } else if (!met && mState.celebrated) {
        mState.celebrated = false;   // an undo took the step back down
    }

    const uint16_t lifetime = liveLifetime();
    for (uint8_t b = 0; b < kBadgeCount; ++b) {
        if (!(mState.badges & (1u << b)) && lifetime >= kSessionBadges[b].sessions) {
            mState.badges = static_cast<uint8_t>(mState.badges | (1u << b));
            ev.add(EventKind::Badge, b);
        }
    }

    const uint8_t q = qualifying();
    if (met && mState.bestWeek > 0 && q > mState.bestWeek && !mState.bestCelebrated) {
        mState.bestCelebrated = true;
        ev.add(EventKind::BestWeek, q);
    }
}

bool StreakModel::logManual(Kind kind, bool yesterday, int32_t nowDay, uint32_t secondOfDay, Events& ev)
{
    if (mState.period == State::kNoPeriod || mState.sessionCount >= State::kMaxSessions) {
        return false;
    }
    const int32_t day = maxOf(nowDay, mState.lastDay) - (yesterday ? 1 : 0);
    if (day < mState.weekStartDay || day >= mState.weekStartDay + 7) {
        return false;   // today or yesterday, within this week (S7)
    }
    Found f;
    f.appKey     = 0;
    f.localStart = static_cast<uint32_t>(day) * static_cast<uint32_t>(kSecondsDay) + secondOfDay % kSecondsDay;
    f.localDay   = day;
    f.kind       = kind;
    f.minutes    = 0;

    Session s;
    s.localStart = f.localStart;
    s.kind       = kind;
    s.app        = kManualApp;
    s.flags      = Session::kManual;
    uint8_t at   = mState.sessionCount;
    while (at > 0 && mState.sessions[at - 1].localStart > s.localStart) {
        mState.sessions[at] = mState.sessions[at - 1];
        --at;
    }
    mState.sessions[at] = s;
    ++mState.sessionCount;
    if (status(at) == Status::Counts) {
        ev.add(EventKind::SessionFound, static_cast<uint8_t>(kind), 0);
    }
    afterChange(ev);
    return true;
}

bool StreakModel::undo(uint8_t index, Events& ev)
{
    if (index >= mState.sessionCount || !(mState.sessions[index].flags & Session::kManual)) {
        return false;
    }
    for (uint8_t i = index; i + 1 < mState.sessionCount; ++i) {
        mState.sessions[i] = mState.sessions[i + 1];
    }
    --mState.sessionCount;
    afterChange(ev);
    return true;
}

bool StreakModel::toggleExclude(uint8_t index, Events& ev)
{
    if (index >= mState.sessionCount || (mState.sessions[index].flags & Session::kManual)) {
        return false;
    }
    mState.sessions[index].flags ^= Session::kExcluded;
    afterChange(ev);
    return true;
}

void StreakModel::setGoal(const Goal& goalIn, int32_t nowDay, Events& ev)
{
    const Goal g = goalIn.sane();
    if (mState.period == State::kNoPeriod) {
        mState.goal = g;   // nothing judged yet: it applies at once
        return;
    }
    if (g.weekStart == mState.goal.weekStart) {
        mState.pending    = g;
        mState.hasPending = g != mState.goal;
        return;
    }

    // A new start day closes this week now (S8): achieved if it met the target,
    // otherwise void. A new week begins today under the new start day.
    const int32_t effNow   = maxOf(nowDay, mState.lastDay);
    const int32_t period   = WeekMath::periodOf(effNow, g.weekStart);
    const int32_t newStart = WeekMath::periodStartDay(period, g.weekStart);
    const uint8_t q        = qualifying();
    const bool    achieved = q >= mState.goal.target;

    Session carried[State::kMaxSessions];
    uint8_t carriedCount = 0;
    if (achieved) {
        closeWeek(ev, true);
    } else {
        // Void: nothing committed. Its sessions on days the new week covers move over.
        addHistory(q, mState.goal.target, Outcome::Void);
        for (uint8_t i = 0; i < mState.sessionCount; ++i) {
            if (static_cast<int32_t>(mState.sessions[i].localStart / kSecondsDay) >= newStart) {
                carried[carriedCount++] = mState.sessions[i];
            }
        }
    }
    const bool trial  = g.target != mState.goal.target || g.scope != mState.goal.scope;
    mState.goal       = g;
    mState.hasPending = false;
    startWeek(period, newStart, trial);
    for (uint8_t i = 0; i < carriedCount; ++i) {
        mState.sessions[mState.sessionCount++] = carried[i];
    }
    afterChange(ev);
}

// -- The home view ----------------------------------------------------------------------------

HomeView StreakModel::view(int32_t nowDay) const
{
    HomeView v;
    const uint8_t q  = qualifying();
    const bool    met = q >= mState.goal.target;
    v.weeksAchieved  = liveWeeks();
    v.streakWeeks    = liveStreak();
    v.target         = mState.goal.target;
    v.sessions       = q;
    v.shields        = mState.shields;
    v.lastWeek       = mState.lastWeekCount;
    const int32_t effNow = maxOf(nowDay, mState.lastDay);
    const int32_t left   = mState.weekStartDay + 7 - effNow;
    v.daysLeft           = static_cast<uint8_t>(left < 1 ? 1 : (left > 7 ? 7 : left));
    const uint8_t remaining = met ? 0 : static_cast<uint8_t>(mState.goal.target - q);
    if (met) {
        v.mood = Mood::Done;
    } else if (mState.trial) {
        v.mood = Mood::Trial;
    } else if (remaining >= v.daysLeft) {
        v.mood = Mood::AtRisk;
    } else {
        v.mood = Mood::Climbing;
    }
    return v;
}

} // namespace Streak
