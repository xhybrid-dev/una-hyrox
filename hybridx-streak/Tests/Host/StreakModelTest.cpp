/**
 * Host tests for Streak::StreakModel: the PLAN 6.6 list (Gate 1).
 * Dates are local day numbers; weeks start on Monday unless a test says not.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

#include "Summits.hpp"
#include "StreakModel.hpp"
#include "WeekMath.hpp"

using namespace Streak;

namespace
{

int32_t day(int y, int m, int d)
{
    return WeekMath::daysFromCivil(y, static_cast<uint32_t>(m), static_cast<uint32_t>(d));
}

// Monday 21 September 2026 starts the week these tests live in.
const int32_t kMon = day(2026, 9, 21);

Found session(int32_t d, Kind kind = Kind::Run, uint16_t minutes = 30, const char* app = "Running", int hour = 8)
{
    Found f;
    std::strncpy(f.app, app, sizeof(f.app) - 1);
    f.appKey     = ActivityScanner::appKey(app);
    f.localDay   = d;
    f.localStart = static_cast<uint32_t>(d) * 86400u + static_cast<uint32_t>(hour) * 3600u;
    f.kind       = kind;
    f.minutes    = minutes;
    return f;
}

bool has(const Events& ev, EventKind k)
{
    for (uint8_t i = 0; i < ev.count; ++i) {
        if (ev.items[i].kind == k) {
            return true;
        }
    }
    return false;
}

int countOf(const Events& ev, EventKind k)
{
    int n = 0;
    for (uint8_t i = 0; i < ev.count; ++i) {
        n += ev.items[i].kind == k ? 1 : 0;
    }
    return n;
}

const Event* find(const Events& ev, EventKind k)
{
    for (uint8_t i = 0; i < ev.count; ++i) {
        if (ev.items[i].kind == k) {
            return &ev.items[i];
        }
    }
    return nullptr;
}

/// Open the app on @p d with @p found (sorted), returning the events.
Events open(StreakModel& m, int32_t d, std::vector<Found> found = {})
{
    Events ev;
    m.update(d, found.data(), found.size(), ev);
    return ev;
}

/// A model installed on the Monday, its trial week achieved with three runs.
StreakModel achievedFirstWeek()
{
    StreakModel m;
    m.reset(Goal {});
    open(m, kMon, {session(kMon), session(kMon + 1), session(kMon + 2)});
    return m;
}

/// Achieve week @p w (0 = kMon's week) with three sessions, opened on its Sunday.
void achieveWeek(StreakModel& m, int w)
{
    const int32_t start = kMon + 7 * w;
    open(m, start + 6, {session(start), session(start + 2), session(start + 4)});
}

} // namespace

TEST(StreakModel, FirstOpenIsATrialWeekScanningThisWeekOnly)
{
    StreakModel m;
    m.reset(Goal {});
    int32_t from = 0, to = 0;
    m.scanWindow(kMon + 3, from, to);   // Thursday
    EXPECT_EQ(from, kMon);
    EXPECT_EQ(to, kMon + 3);

    const Events ev = open(m, kMon + 3, {session(kMon + 1)});
    EXPECT_EQ(countOf(ev, EventKind::SessionFound), 1);
    const HomeView v = m.view(kMon + 3);
    EXPECT_EQ(v.mood, Mood::Trial);
    EXPECT_EQ(v.sessions, 1);
    EXPECT_EQ(v.daysLeft, 4);   // Thursday to Sunday
    EXPECT_EQ(m.state().installDay, kMon + 3);
}

TEST(StreakModel, MeetingTheTargetStepsUpAtOnce)
{
    StreakModel m;
    m.reset(Goal {});
    open(m, kMon, {session(kMon), session(kMon + 1)});
    EXPECT_EQ(m.view(kMon + 1).weeksAchieved, 0);

    const Events ev = open(m, kMon + 2, {session(kMon + 2)});
    ASSERT_TRUE(has(ev, EventKind::StepUp));
    const HomeView v = m.view(kMon + 2);
    EXPECT_EQ(v.weeksAchieved, 1);
    EXPECT_EQ(v.streakWeeks, 1);
    EXPECT_EQ(v.mood, Mood::Done);
    // A fourth session is a bonus, not a second step.
    const Events more = open(m, kMon + 3, {session(kMon + 3)});
    EXPECT_FALSE(has(more, EventKind::StepUp));
    EXPECT_EQ(m.view(kMon + 3).weeksAchieved, 1);
}

TEST(StreakModel, AnAchievedWeekIsCommittedAndTheStreakCarriesOn)
{
    StreakModel m = achievedFirstWeek();
    achieveWeek(m, 1);
    const Events ev = open(m, kMon + 14);
    EXPECT_EQ(m.state().streak, 2);
    EXPECT_EQ(m.state().weeksAchieved, 2);
    EXPECT_EQ(m.state().lifetime, 6);
    const Event* r = find(ev, EventKind::WeekResult);
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->a, 3);
    EXPECT_EQ(r->b & 0xFF, 3);
    EXPECT_EQ(static_cast<Outcome>(r->b >> 8), Outcome::Achieved);
    EXPECT_EQ(m.view(kMon + 14).mood, Mood::Climbing);
}

TEST(StreakModel, CreditBeforeJudging)
{
    // A Sunday session found on the following Tuesday counts for its own week.
    StreakModel m;
    m.reset(Goal {});
    open(m, kMon, {session(kMon), session(kMon + 1)});
    open(m, kMon + 8, {session(kMon + 6)});
    EXPECT_EQ(m.state().weeksAchieved, 1);
    EXPECT_EQ(m.state().streak, 1);
    EXPECT_EQ(m.view(kMon + 8).sessions, 0);
}

TEST(StreakModel, ATrialWeekMissedDoesNoHarm)
{
    StreakModel m;
    m.reset(Goal {});
    open(m, kMon, {session(kMon)});
    const Events ev = open(m, kMon + 7);
    EXPECT_FALSE(has(ev, EventKind::ShieldOffer));
    EXPECT_FALSE(has(ev, EventKind::StreakReset));
    EXPECT_EQ(m.state().history[0].outcome, Outcome::Trial);
    EXPECT_FALSE(m.state().trial);
}

TEST(StreakModel, MissedWithNoStreakIsJustMissed)
{
    StreakModel m;
    m.reset(Goal {});
    open(m, kMon);
    open(m, kMon + 7);    // the trial week goes by
    const Events ev = open(m, kMon + 14);
    EXPECT_FALSE(has(ev, EventKind::ShieldOffer));
    EXPECT_FALSE(has(ev, EventKind::StreakReset));
    EXPECT_EQ(m.state().history[1].outcome, Outcome::Missed);
}

TEST(StreakModel, MissedWithShieldsOffersThem)
{
    StreakModel m = achievedFirstWeek();
    for (int w = 1; w < 4; ++w) {
        achieveWeek(m, w);
    }
    open(m, kMon + 28);   // week 4 begins: 4 weeks achieved, one shield earned
    ASSERT_EQ(m.state().streak, 4);
    ASSERT_EQ(m.state().shields, 1);

    const Events ev = open(m, kMon + 35);   // week 4 had nothing
    const Event* offer = find(ev, EventKind::ShieldOffer);
    ASSERT_NE(offer, nullptr);
    EXPECT_EQ(offer->a, 1);
    EXPECT_EQ(offer->b, 4);
    EXPECT_EQ(m.view(kMon + 35).streakWeeks, 4);   // what's at stake is shown

    Events decided;
    m.decideShields(true, decided);
    EXPECT_EQ(m.state().streak, 4);
    EXPECT_EQ(m.state().shields, 0);
    EXPECT_EQ(m.state().pendingMissed, 0);
    EXPECT_EQ(m.state().history[(m.state().historyNext + State::kHistory - 1) % State::kHistory].outcome,
              Outcome::Shielded);
}

TEST(StreakModel, LettingItGoResets)
{
    StreakModel m = achievedFirstWeek();
    for (int w = 1; w < 4; ++w) {
        achieveWeek(m, w);
    }
    open(m, kMon + 35);
    Events ev;
    m.decideShields(false, ev);
    ASSERT_TRUE(has(ev, EventKind::StreakReset));
    EXPECT_EQ(find(ev, EventKind::StreakReset)->b, 4);
    EXPECT_EQ(m.state().streak, 0);
    EXPECT_EQ(m.state().shields, 1);             // kept
    EXPECT_EQ(m.state().weeksAchieved, 4);       // the climb is safe (S10)
}

TEST(StreakModel, NotEnoughShieldsResetsAndSaysWhy)
{
    StreakModel m = achievedFirstWeek();
    achieveWeek(m, 1);
    open(m, kMon + 14);
    ASSERT_EQ(m.state().shields, 0);
    const Events ev = open(m, kMon + 21);   // week 2 missed
    ASSERT_TRUE(has(ev, EventKind::StreakReset));
    EXPECT_EQ(find(ev, EventKind::StreakReset)->b, 2);
    EXPECT_FALSE(has(ev, EventKind::ShieldOffer));
    EXPECT_EQ(m.state().streak, 0);
}

TEST(StreakModel, CatchUpOfOneTwoAndFiveMissedWeeks)
{
    auto withShields = [](int shields) {
        StreakModel m = achievedFirstWeek();
        const int   weeks = shields == 0 ? 2 : 4 * shields;
        for (int w = 1; w < weeks; ++w) {
            achieveWeek(m, w);
        }
        open(m, kMon + 7 * weeks);
        return m;
    };
    struct Case { int shields, missed; bool offer; };
    for (const Case c : {Case {0, 1, false}, Case {1, 1, true}, Case {2, 2, true}, Case {1, 2, false},
                         Case {2, 5, false}}) {
        StreakModel   m     = withShields(c.shields);
        const int32_t first = m.state().weekStartDay;
        ASSERT_EQ(m.state().shields, c.shields);
        const Events ev = open(m, first + 7 * c.missed);
        EXPECT_EQ(has(ev, EventKind::ShieldOffer), c.offer) << c.shields << " shields, " << c.missed << " missed";
        EXPECT_EQ(has(ev, EventKind::StreakReset), !c.offer);
        if (c.offer) {
            EXPECT_EQ(find(ev, EventKind::ShieldOffer)->a, c.missed);
        }
    }
}

TEST(StreakModel, AnAchievedWeekBetweenMissesStaysWithTheChain)
{
    StreakModel m = achievedFirstWeek();
    for (int w = 1; w < 8; ++w) {
        achieveWeek(m, w);   // 8 weeks, 2 shields
    }
    open(m, kMon + 56);        // week 8 begins
    open(m, kMon + 63);        // week 8 missed: offer, left undecided
    achieveWeek(m, 9);
    const Events ev = open(m, kMon + 70);   // week 10 begins
    ASSERT_TRUE(has(ev, EventKind::ShieldOffer));
    EXPECT_EQ(find(ev, EventKind::ShieldOffer)->a, 1);
    EXPECT_EQ(find(ev, EventKind::ShieldOffer)->b, 9);   // 8 before, 1 after

    StreakModel letGo = m;
    Events      e1;
    letGo.decideShields(false, e1);
    EXPECT_EQ(letGo.state().streak, 1);   // the week after the miss survives

    Events e2;
    m.decideShields(true, e2);
    EXPECT_EQ(m.state().streak, 9);
}

TEST(StreakModel, ShieldsComeEveryFourWeeksUpToTwo)
{
    StreakModel m = achievedFirstWeek();
    int         earned = 0;
    for (int w = 1; w < 13; ++w) {
        achieveWeek(m, w);
        const Events ev = open(m, kMon + 7 * (w + 1));
        earned += countOf(ev, EventKind::ShieldEarned);
    }
    EXPECT_EQ(m.state().weeksAchieved, 13);
    EXPECT_EQ(m.state().shields, 2);
    EXPECT_EQ(earned, 2);   // at 4 and 8 weeks; the one at 12 is over the cap
}

TEST(StreakModel, TheDedupRingCountsAFileOnce)
{
    StreakModel m;
    m.reset(Goal {});
    const Found f = session(kMon + 1);
    open(m, kMon + 1, {f});
    EXPECT_TRUE(m.seen(f.appKey, f.localStart));
    open(m, kMon + 2, {f});   // a scan that finds it again
    EXPECT_EQ(m.view(kMon + 2).sessions, 1);
}

TEST(StreakModel, ACrashBetweenCountingAndSavingCountsOnce)
{
    // Counted, then the save never happened: the next open starts from the
    // saved state and finds the same files. The result is the same as if
    // nothing had gone wrong.
    StreakModel saved;
    saved.reset(Goal {});
    open(saved, kMon);
    StreakModel lost = saved;
    open(lost, kMon + 2, {session(kMon + 1), session(kMon + 2)});
    StreakModel recovered = saved;
    open(recovered, kMon + 2, {session(kMon + 1), session(kMon + 2)});
    EXPECT_EQ(recovered.view(kMon + 2).sessions, lost.view(kMon + 2).sessions);
    EXPECT_EQ(recovered.view(kMon + 2).sessions, 2);
}

TEST(StreakModel, EvictedKeysStillCountAsSeen)
{
    StreakModel m;
    m.reset(Goal {});
    open(m, kMon);
    std::vector<Found> many;
    for (int i = 0; i < 70; ++i) {
        many.push_back(session(kMon + i / 12, Kind::Run, 30, "Running", 6 + i % 12));
    }
    open(m, kMon + 6, many);
    EXPECT_EQ(m.state().ringCount, State::kRing);
    EXPECT_TRUE(m.seen(many[0].appKey, many[0].localStart));   // evicted, but below the floor
    EXPECT_TRUE(m.seen(many[69].appKey, many[69].localStart));
    EXPECT_FALSE(m.seen(many[0].appKey, many[69].localStart + 60));
}

TEST(StreakModel, MinimumDurationScopeAndOnePerDay)
{
    StreakModel m;
    Goal        g;
    g.minMinutes = 10;
    g.onePerDay  = true;
    m.reset(g);
    open(m, kMon + 2, {session(kMon, Kind::Walk, 6), session(kMon + 1, Kind::Run, 30, "Running", 7),
                       session(kMon + 1, Kind::Ride, 45, "Cycling", 18)});
    EXPECT_EQ(m.status(0), Status::TooShort);
    EXPECT_EQ(m.status(1), Status::Counts);
    EXPECT_EQ(m.status(2), Status::SameDay);
    EXPECT_EQ(m.qualifying(), 1);

    Goal runsOnly = g;
    runsOnly.scope     = static_cast<uint8_t>(Kind::Run);
    runsOnly.onePerDay = false;
    StreakModel r;
    r.reset(runsOnly);
    open(r, kMon + 2, {session(kMon + 1, Kind::Run), session(kMon + 1, Kind::Ride, 45, "Cycling", 18)});
    EXPECT_EQ(r.status(1), Status::OutOfScope);
    EXPECT_EQ(r.qualifying(), 1);
}

TEST(StreakModel, ManualLogsUndoAndExclude)
{
    StreakModel m;
    m.reset(Goal {});
    open(m, kMon + 2, {session(kMon), session(kMon + 1)});
    Events ev;
    ASSERT_TRUE(m.logManual(Kind::Row, false, kMon + 2, 12 * 3600, ev));
    EXPECT_TRUE(has(ev, EventKind::StepUp));
    EXPECT_EQ(m.view(kMon + 2).weeksAchieved, 1);

    // Undo takes the step back down; logging again steps up again.
    Events undone;
    const uint8_t manual = 2;
    ASSERT_TRUE(m.state().sessions[manual].flags & Session::kManual);
    ASSERT_TRUE(m.undo(manual, undone));
    EXPECT_EQ(m.view(kMon + 2).weeksAchieved, 0);
    EXPECT_FALSE(m.undo(0, undone));   // not a manual log

    Events yesterday;
    ASSERT_TRUE(m.logManual(Kind::Strength, true, kMon + 2, 19 * 3600, yesterday));
    EXPECT_TRUE(has(yesterday, EventKind::StepUp));

    // Excluding an automatic session takes it out of the count.
    Events ex;
    ASSERT_TRUE(m.toggleExclude(0, ex));
    EXPECT_EQ(m.status(0), Status::Excluded);
    EXPECT_EQ(m.view(kMon + 2).weeksAchieved, 0);
    ASSERT_TRUE(m.toggleExclude(0, ex));
    EXPECT_EQ(m.view(kMon + 2).weeksAchieved, 1);
}

TEST(StreakModel, YesterdayMustBeInThisWeek)
{
    StreakModel m;
    m.reset(Goal {});
    open(m, kMon);
    Events ev;
    EXPECT_FALSE(m.logManual(Kind::Run, true, kMon, 3600, ev));   // Sunday was last week
    EXPECT_TRUE(m.logManual(Kind::Run, false, kMon, 3600, ev));
}

TEST(StreakModel, GoalChangesApplyNextWeekAndANewTargetIsATrial)
{
    StreakModel m = achievedFirstWeek();
    achieveWeek(m, 1);
    open(m, kMon + 14);
    Goal g   = m.state().goal;
    g.target = 5;
    Events ev;
    m.setGoal(g, kMon + 15, ev);
    EXPECT_EQ(m.state().goal.target, 3);
    EXPECT_TRUE(m.state().hasPending);

    open(m, kMon + 21);   // week 2 missed under the old goal: a trial? no, a real miss
    EXPECT_EQ(m.state().goal.target, 5);
    EXPECT_TRUE(m.state().trial);
}

TEST(StreakModel, AWeekStartChangeClosesTheWeekEarly)
{
    // Achieved: committed now, and the new week starts empty.
    StreakModel a = achievedFirstWeek();
    Goal        sunday = a.state().goal;
    sunday.weekStart   = 0;
    Events ev;
    a.setGoal(sunday, kMon + 3, ev);   // Thursday
    EXPECT_EQ(a.state().weeksAchieved, 1);
    EXPECT_EQ(a.state().weekStartDay, kMon - 1);   // the Sunday before
    EXPECT_EQ(a.view(kMon + 3).sessions, 0);

    // Not met: void, and its sessions on the new week's days carry over.
    StreakModel v;
    v.reset(Goal {});
    open(v, kMon + 3, {session(kMon + 1), session(kMon + 3)});
    Goal sat      = v.state().goal;
    sat.weekStart = 6;   // Saturday: the new week began on Saturday 19th
    Events ev2;
    v.setGoal(sat, kMon + 3, ev2);
    EXPECT_EQ(v.state().weeksAchieved, 0);
    EXPECT_EQ(v.state().history[0].outcome, Outcome::Void);
    EXPECT_EQ(v.view(kMon + 3).sessions, 2);
}

TEST(StreakModel, TheClockGoingBackNeverUnrollsAWeek)
{
    StreakModel m = achievedFirstWeek();
    open(m, kMon + 8);
    const int32_t period = m.state().period;
    open(m, kMon + 2);   // the clock went back a week
    EXPECT_EQ(m.state().period, period);
    int32_t from = 0, to = 0;
    m.scanWindow(kMon + 2, from, to);
    EXPECT_EQ(to, kMon + 2);
}

TEST(StreakModel, SummitsAndBadges)
{
    StreakModel m = achievedFirstWeek();
    for (int w = 1; w < 3; ++w) {
        achieveWeek(m, w);
    }
    open(m, kMon + 21);
    // The fourth achieved week reaches Arthur's Seat, live.
    open(m, kMon + 22, {session(kMon + 21), session(kMon + 22)});
    Events ev;
    m.logManual(Kind::Run, false, kMon + 22, 60000, ev);
    const Event* summit = find(ev, EventKind::Summit);
    ASSERT_NE(summit, nullptr);
    EXPECT_EQ(summit->a, 0);   // Arthur's Seat
    // 12 sessions so far: the 10-session badge came earlier.
    EXPECT_TRUE(m.state().badges & 1u);
}

TEST(StreakModel, BestWeekIsCelebratedOnce)
{
    StreakModel m = achievedFirstWeek();   // best week 3 once committed
    open(m, kMon + 7);
    open(m, kMon + 9, {session(kMon + 7), session(kMon + 8), session(kMon + 9)});
    const Events ev = open(m, kMon + 10, {session(kMon + 10)});
    ASSERT_TRUE(has(ev, EventKind::BestWeek));
    EXPECT_EQ(find(ev, EventKind::BestWeek)->a, 4);
    const Events again = open(m, kMon + 11, {session(kMon + 11)});
    EXPECT_FALSE(has(again, EventKind::BestWeek));
}

TEST(StreakModel, HistoryKeepsTheLast52Weeks)
{
    StreakModel m;
    m.reset(Goal {});
    open(m, kMon);
    for (int w = 1; w <= 60; ++w) {
        open(m, kMon + 7 * w);
    }
    EXPECT_EQ(m.state().historyCount, State::kHistory);
    EXPECT_EQ(m.state().historyNext, 60 % State::kHistory);
}

TEST(StreakModel, EventsKeepTheImportantOnesWhenFull)
{
    Events ev;
    for (int i = 0; i < 10; ++i) {
        ev.add(EventKind::SessionFound);
    }
    ev.add(EventKind::StepUp);
    ev.add(EventKind::Summit);
    EXPECT_EQ(ev.count, Events::kMax);
    EXPECT_TRUE(has(ev, EventKind::StepUp));
    EXPECT_TRUE(has(ev, EventKind::Summit));
    EXPECT_EQ(ev.dropped, 4);
}

TEST(StreakModel, AtRiskWhenTheSessionsLeftMatchTheDays)
{
    StreakModel m = achievedFirstWeek();
    open(m, kMon + 7);
    EXPECT_EQ(m.view(kMon + 7).mood, Mood::Climbing);   // 3 to do in 7 days
    EXPECT_EQ(m.view(kMon + 11).mood, Mood::AtRisk);    // 3 to do in 3 days
    EXPECT_EQ(m.view(kMon + 11).daysLeft, 3);
}

TEST(StreakModel, GoalIsMadeSane)
{
    Goal g;
    g.target     = 0;
    g.weekStart  = 9;
    g.scope      = 40;
    g.minMinutes = 250;
    const Goal s = g.sane();
    EXPECT_EQ(s.target, 1);
    EXPECT_EQ(s.weekStart, 2);
    EXPECT_EQ(s.scope, kScopeAny);
    EXPECT_EQ(s.minMinutes, 120);
}
