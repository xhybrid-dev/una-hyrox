#include "PlanRules.hpp"
#include "PlanSim.hpp"
#include "data/PlanData.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>

using namespace XTraining;
using namespace XTraining::PlanRules;

namespace {

// The 10K plan at level 2, 5K 22:00, from Monday 4 January 2027, Tue Wed Fri Sun, long run Sunday.
struct Fixture {
    const PlanDef& def = *PlanData::find(PlanId::TenK);
    PlanState st;
    Paces paces = PaceModel::compute({22 * 60, 0}).paces;
    explicit Fixture(Ymd race = 0)
    {
        st.setup.plan = PlanId::TenK;
        st.setup.level = 2;
        st.setup.fiveKSec = 22 * 60;
        st.setup.fiveKDate = 20270104u;
        st.setup.raceDate = race;
        st.calendar = PlanCalendar::make({20270104u, race, def.weekCount, def.minWeeks});
    }
    Ymd at(int week, int day) const { return Date::addDays(20270104u, week * 7 + day); }
    DaySession on(Ymd d) const
    {
        DaySession s;
        EXPECT_TRUE(sessionOn(st, def, paces, d, s));
        return s;
    }
    void run(Ymd d, DayStatus s = DayStatus::Done)
    {
        const DaySession x = on(d);
        st.record({d, x.asRun.kind, s, 1});
    }
    /// Every session of the weeks before run week @p week is run, and settled.
    void doneBefore(int week)
    {
        for (int d = 0; d < week * 7; ++d) {
            const Ymd date = at(0, d);
            if (on(date).asRun.kind != Kind::Rest && st.find(date) == nullptr) {
                run(date);
            }
        }
        settle(st, def, paces, at(week, 0));
    }
};

std::string readFile(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

}  // namespace

TEST(PlanRules, WithNoMemoryTheSessionIsThePlans)
{
    Fixture f;
    for (int d = 0; d < 28; ++d) {
        DaySession mine, theirs;
        const Ymd date = f.at(0, d);
        ASSERT_TRUE(sessionOn(f.st, f.def, f.paces, date, mine));
        ASSERT_TRUE(PlanSchedule::day(f.st.setup, f.def, f.st.calendar, f.paces, date, theirs));
        EXPECT_EQ(mine.asRun.kind, theirs.asRun.kind) << d;
        EXPECT_EQ(mine.key, theirs.key) << d;
    }
}

TEST(PlanRules, MissedDaysAreLoggedOnceAndRestDaysNever)
{
    Fixture f;
    const Settled a = settle(f.st, f.def, f.paces, f.at(0, 7));   // the whole of week 1, nothing run
    EXPECT_EQ(a.missed, 4);
    EXPECT_EQ(f.st.logCount(), 4);
    EXPECT_EQ(f.st.find(f.at(0, 0)), nullptr);               // Monday rests
    EXPECT_EQ(f.st.find(f.at(0, 1))->status, DayStatus::Missed);
    const Settled b = settle(f.st, f.def, f.paces, f.at(0, 7));   // again: nothing new
    EXPECT_EQ(b.missed, 0);
    EXPECT_EQ(f.st.logCount(), 4);
}

TEST(PlanRules, DaysBeforeThePlanStartsAreNotMissed)
{
    Fixture f;
    const Settled s = settle(f.st, f.def, f.paces, 20270102u);   // two days before the start
    EXPECT_EQ(s.missed, 0);
    EXPECT_EQ(f.st.logCount(), 0);
}

TEST(PlanRules, APlanSetUpMidWeekIsSettledFromItsSetUpDayNotFromTheCalendarStart)
{
    // Set up on the Wednesday of the first week (the calendar starts on the Monday): the Tuesday
    // before was never asked of the runner. The service sets settledTo to the set-up day.
    Fixture f;
    f.st.settledTo = f.at(0, 2);
    const Settled s = settle(f.st, f.def, f.paces, f.at(0, 4));   // Friday: Wed (the set-up day) is judged
    EXPECT_EQ(f.st.find(f.at(0, 1)), nullptr) << "Tuesday, before the set-up";
    ASSERT_NE(f.st.find(f.at(0, 2)), nullptr);
    EXPECT_EQ(f.st.find(f.at(0, 2))->status, DayStatus::Missed);
    EXPECT_EQ(s.missed, 1);
    // Without it (the old behaviour, a plan with settledTo 0) Tuesday would be logged too.
    Fixture g;
    EXPECT_EQ(settle(g.st, g.def, g.paces, g.at(0, 4)).missed, 2);
}

TEST(PlanRules, PausingAfterTodaysRunKeepsTodaysSession)
{
    Fixture f;
    f.run(f.at(0, 1));                       // Tuesday run
    pause(f.st, f.at(0, 1));                 // paused the same day, after the run
    EXPECT_NE(f.on(f.at(0, 1)).asRun.kind, Kind::Rest) << "today's counted session stays";
    EXPECT_EQ(f.on(f.at(0, 2)).asRun.kind, Kind::Rest);   // tomorrow rests
    // A day that was not run still rests from the pause.
    Fixture g;
    pause(g.st, g.at(0, 1));
    EXPECT_EQ(g.on(g.at(0, 1)).asRun.kind, Kind::Rest);
}

TEST(PlanRules, EasingHoldsThePlanBySendingNextWeekBackToThisWeeksPlanWeek)
{
    Fixture f;   // no race: the plan grows by a week
    f.doneBefore(2);
    const DayPlace here = PlanCalendar::locate(f.st.calendar, f.at(2, 2));
    ASSERT_EQ(here.runWeek, 2);
    const uint8_t weeksBefore = f.st.calendar.weeks;
    Settled::Denied why = Settled::Denied::None;
    EXPECT_TRUE(easeNextWeek(f.st, f.def, f.at(2, 2), &why));
    EXPECT_EQ(why, Settled::Denied::None);
    EXPECT_EQ(f.st.calendar.weeks, weeksBefore + 1);
    // Run week 3 now runs the plan week that run week 2 runs.
    EXPECT_EQ(PlanCalendar::locate(f.st.calendar, f.at(3, 2)).planWeek, here.planWeek);
    // Doing it twice for the same week has no more to give.
    EXPECT_FALSE(easeNextWeek(f.st, f.def, f.at(2, 3), &why));
    EXPECT_EQ(why, Settled::Denied::NoRoom);
}

TEST(PlanRules, CanEaseSaysWhatEasingWouldDoWithoutDoingIt)
{
    Fixture f;
    f.doneBefore(2);
    const PlanState before = f.st;
    Settled::Denied why = Settled::Denied::PlanEnd;
    EXPECT_TRUE(canEase(f.st, f.def, f.at(2, 2), &why));
    EXPECT_EQ(why, Settled::Denied::None);
    EXPECT_EQ(f.st.calendar.weeks, before.calendar.weeks);   // nothing changed
    EXPECT_EQ(f.st.calendar.adjCount, before.calendar.adjCount);
    ASSERT_TRUE(easeNextWeek(f.st, f.def, f.at(2, 2)));
    EXPECT_FALSE(canEase(f.st, f.def, f.at(2, 3), &why));   // done once: no more for this week
    EXPECT_EQ(why, Settled::Denied::NoRoom);
    // With a race, the last weeks before it refuse, as easeNextWeek does.
    Fixture race(20270314u);
    race.doneBefore(race.st.calendar.weeks - 2);
    EXPECT_FALSE(canEase(race.st, race.def, race.at(race.st.calendar.weeks - 2, 2), &why));
    EXPECT_NE(why, Settled::Denied::None);
}

TEST(PlanRules, EasingIsRefusedInTheLastWeeksBeforeARaceAndAtThePlansEnd)
{
    Fixture race(20270314u);   // a Sunday 10 weeks on
    ASSERT_TRUE(race.st.calendar.usable());
    race.doneBefore(race.st.calendar.weeks - 2);
    Settled::Denied why = Settled::Denied::None;
    EXPECT_FALSE(easeNextWeek(race.st, race.def, race.at(race.st.calendar.weeks - 2, 2), &why));
    EXPECT_NE(why, Settled::Denied::None);
    // On the last week there is no next week to hold.
    Settled::Denied last = Settled::Denied::None;
    EXPECT_FALSE(easeNextWeek(race.st, race.def, race.at(race.st.calendar.weeks - 1, 2), &last));
    EXPECT_NE(last, Settled::Denied::None);
}

TEST(PlanRules, EasingNeedsAPlanUnderWayAndATodayInIt)
{
    Fixture f;
    Settled::Denied why = Settled::Denied::None;
    EXPECT_FALSE(easeNextWeek(f.st, f.def, 20261201u, &why));     // before the plan
    EXPECT_EQ(why, Settled::Denied::NoRoom);
    EXPECT_FALSE(easeNextWeek(f.st, f.def, 0, &why));              // no date
    PlanState none;
    EXPECT_FALSE(easeNextWeek(none, f.def, f.at(1, 1), &why));    // no plan
    EXPECT_FALSE(easeNextWeek(none, f.def, f.at(1, 1)));            // and no reason wanted
}

TEST(PlanRules, AMissedKeySessionTakesALaterQualitysPlace)
{
    Fixture f;
    f.doneBefore(5);
    // Week 6: Tuesday's intervals are the key, Friday's cruise the other quality.
    ASSERT_TRUE(f.on(f.at(5, 1)).key);
    ASSERT_EQ(f.on(f.at(5, 1)).asRun.kind, Kind::Intervals);
    ASSERT_EQ(f.on(f.at(5, 4)).asRun.kind, Kind::Cruise);
    const Settled s = settle(f.st, f.def, f.paces, f.at(5, 2));
    EXPECT_EQ(s.keyMoved, 1);
    EXPECT_EQ(f.on(f.at(5, 4)).asRun.kind, Kind::Intervals);
    EXPECT_TRUE(f.on(f.at(5, 4)).key);
    // Run on Friday: it counts.
    f.run(f.at(5, 4));
    EXPECT_TRUE(f.st.find(f.at(5, 4)) != nullptr);
}

TEST(PlanRules, AMissedEasyRunIsDropped)
{
    Fixture f;
    const Settled s = settle(f.st, f.def, f.paces, f.at(1, 3));   // week 2's Wednesday not run
    EXPECT_EQ(s.keyMoved, 0);
    EXPECT_EQ(f.st.overrideCount, 0);
}

TEST(PlanRules, AMissedLongRunIsOfferedTheNextDayIfEasyOrRestAndOnlyOnce)
{
    Fixture f;
    for (int w = 0; w < 2; ++w) {
        for (int d = 0; d < 7; ++d) {
            if (!(w == 1 && d == 6)) {
                const DaySession s = f.on(f.at(w, d));
                if (s.asRun.kind != Kind::Rest) {
                    f.run(f.at(w, d));
                }
            }
        }
    }
    settle(f.st, f.def, f.paces, f.at(2, 0));
    const Offer o = offer(f.st, f.def, f.paces, f.at(2, 0));
    ASSERT_EQ(o.kind, Offer::Kind::LongRun);
    EXPECT_EQ(o.from, f.at(1, 6));
    acceptOffer(f.st, o, f.at(2, 0));
    EXPECT_EQ(f.on(f.at(2, 0)).asRun.kind, Kind::Long);
    // Declined instead: not offered again.
    Fixture g;
    for (int d = 0; d < 7; ++d) {
        if (g.on(g.at(0, d)).asRun.kind != Kind::Rest && d != 6) {
            g.run(g.at(0, d));
        }
    }
    settle(g.st, g.def, g.paces, g.at(1, 0));
    const Offer o2 = offer(g.st, g.def, g.paces, g.at(1, 0));
    ASSERT_EQ(o2.kind, Offer::Kind::LongRun);
    declineOffer(g.st, o2);
    EXPECT_EQ(offer(g.st, g.def, g.paces, g.at(1, 0)).kind, Offer::Kind::None);
}

TEST(PlanRules, ALongRunIsNotOfferedOnAHardDay)
{
    Fixture f;
    // Week 1 Sunday missed; the next Tuesday's hill reps are hard (not offered on the Monday? it rests).
    settle(f.st, f.def, f.paces, f.at(1, 1));
    EXPECT_EQ(offer(f.st, f.def, f.paces, f.at(1, 1)).kind, Offer::Kind::None);   // Tuesday: hills, and the Sunday is long gone
}

TEST(PlanRules, AWeekLessThanHalfDoneIsRepeatedAndThePlanGrowsWithoutARace)
{
    Fixture f;
    f.doneBefore(2);
    f.run(f.at(2, 1));                                      // week 3: 1 of 4
    const uint8_t weeks = f.st.calendar.weeks;
    const Settled s = settle(f.st, f.def, f.paces, f.at(3, 0));
    EXPECT_TRUE(s.repeated);
    EXPECT_FALSE(s.nothingDone);
    EXPECT_EQ(f.st.calendar.weeks, weeks + 1);
    EXPECT_EQ(PlanCalendar::planWeekOf(f.st.calendar, 3), 2);   // run week 4 runs plan week 3
    EXPECT_EQ(PlanCalendar::planWeekOf(f.st.calendar, 4), 3);   // then on
    EXPECT_EQ(PlanCalendar::planWeekOf(f.st.calendar, weeks), f.def.weekCount - 1);   // the end moved out by a week
}

TEST(PlanRules, HalfDoneIsNotRepeated)
{
    Fixture f;
    f.doneBefore(2);
    f.run(f.at(2, 1));
    f.run(f.at(2, 2));                                      // 2 of 4
    EXPECT_FALSE(settle(f.st, f.def, f.paces, f.at(3, 0)).repeated);
    EXPECT_EQ(f.st.calendar.adjCount, 0);
}

TEST(PlanRules, NothingDoneRepeatsTheLastCutBackWeek)
{
    Fixture g;
    g.doneBefore(4);
    EXPECT_EQ(g.st.calendar.adjCount, 0);
    const Settled t = settle(g.st, g.def, g.paces, g.at(5, 0));   // week 5 (index 4): nothing run
    EXPECT_TRUE(t.repeated);
    EXPECT_TRUE(t.nothingDone);
    EXPECT_EQ(g.def.weeks[PlanCalendar::planWeekOf(g.st.calendar, 5)].phase, Phase::CutBack);   // week 4's
    EXPECT_EQ(PlanCalendar::planWeekOf(g.st.calendar, 5), 3);
    EXPECT_EQ(PlanCalendar::planWeekOf(g.st.calendar, 6), 5);   // and the plan resumes at week 6
}

TEST(PlanRules, WithARaceTheRepeatTakesTheNextWeeksPlaceAndTheEndStands)
{
    Fixture f(20270314u);
    f.doneBefore(4);
    const uint8_t weeks = f.st.calendar.weeks;
    f.run(f.at(4, 1));                                       // week 5: 1 of 4
    const Settled s = settle(f.st, f.def, f.paces, f.at(5, 0));
    EXPECT_TRUE(s.repeated);
    EXPECT_EQ(f.st.calendar.weeks, weeks);                   // cannot grow
    EXPECT_EQ(PlanCalendar::planWeekOf(f.st.calendar, 5), 4);   // week 6 runs plan week 5 again
    EXPECT_EQ(PlanCalendar::planWeekOf(f.st.calendar, 6), 6);   // week 6 of the plan is dropped
    EXPECT_EQ(PlanCalendar::planWeekOf(f.st.calendar, 9), 9);   // the race week is the race week
}

TEST(PlanRules, TheLastWeeksBeforeARaceAreNeverTouched)
{
    Fixture f(20270314u);
    f.doneBefore(7);
    f.run(f.at(7, 1));                                       // week 8: 1 of 4
    const Settled s = settle(f.st, f.def, f.paces, f.at(8, 0));
    EXPECT_FALSE(s.repeated);
    EXPECT_EQ(s.denied, Settled::Denied::LastWeeks);
    EXPECT_EQ(f.st.calendar.adjCount, 0);
}

TEST(PlanRules, WithNoRaceThePlanGrowsTwiceAtMost)
{
    Fixture f;
    f.doneBefore(1);
    const uint8_t weeks = f.st.calendar.weeks;
    f.run(f.at(1, 1));
    settle(f.st, f.def, f.paces, f.at(2, 0));      // week 2 repeated as week 3
    f.run(f.at(2, 1));
    settle(f.st, f.def, f.paces, f.at(3, 0));      // and again as week 4
    EXPECT_EQ(f.st.calendar.weeks, weeks + 2);
    f.run(f.at(3, 1));
    const Settled s = settle(f.st, f.def, f.paces, f.at(4, 0));
    EXPECT_FALSE(s.repeated);
    EXPECT_EQ(s.denied, Settled::Denied::GrowthLimit);
    EXPECT_EQ(f.st.calendar.weeks, weeks + 2);
}

TEST(PlanRules, DoingTomorrowsSessionTodaySwapsThem)
{
    Fixture f;
    const Ymd tue = f.at(2, 1);   // reps
    ASSERT_EQ(f.on(tue).asRun.kind, Kind::Reps);
    ASSERT_EQ(f.on(f.at(2, 2)).asRun.kind, Kind::EasyStrides);
    EXPECT_EQ(doTomorrowToday(f.st, f.def, f.paces, tue), MoveResult::Ok);
    EXPECT_EQ(f.on(tue).asRun.kind, Kind::EasyStrides);
    EXPECT_EQ(f.on(f.at(2, 2)).asRun.kind, Kind::Reps);
    // A day already moved stays moved.
    EXPECT_EQ(doTomorrowToday(f.st, f.def, f.paces, tue), MoveResult::NotAllowed);
}

TEST(PlanRules, DoingTomorrowsHardSessionTodayWhenYesterdayWasHardMakesTodayEasy)
{
    Fixture f;
    // Thursday rests; Friday (week 3) is the tempo run, Wednesday before was easy: allowed as is.
    EXPECT_EQ(doTomorrowToday(f.st, f.def, f.paces, f.at(2, 3)), MoveResult::Ok);
    EXPECT_EQ(f.on(f.at(2, 3)).asRun.kind, Kind::Tempo);
    EXPECT_EQ(f.on(f.at(2, 4)).asRun.kind, Kind::Rest);
    // Now with a hard session done yesterday (week 3 Tuesday's reps run), do Wednesday's... Thursday's
    // tempo would touch it: Tuesday's reps done two days before are apart. Make yesterday hard:
    Fixture g;
    g.run(g.at(2, 3));   // pretend Thursday's (a rest day) was run as the hard session of kind Tempo
    g.st.record(LogEntry{g.at(2, 3), Kind::Tempo, DayStatus::Done, 1});
    // Friday today, Saturday tomorrow (rest): nothing to move.
    EXPECT_EQ(doTomorrowToday(g.st, g.def, g.paces, g.at(2, 4)), MoveResult::NothingToMove);
}

TEST(PlanRules, MovingTodayToTomorrowRestsToday)
{
    Fixture f;
    EXPECT_EQ(moveTodayTomorrow(f.st, f.def, f.paces, f.at(2, 4)), MoveResult::Ok);   // tempo to Saturday
    EXPECT_EQ(f.on(f.at(2, 4)).asRun.kind, Kind::Rest);
    EXPECT_EQ(f.on(f.at(2, 5)).asRun.kind, Kind::Tempo);
    // Tomorrow hard: not allowed (it would lose that session).
    Fixture g;
    EXPECT_EQ(moveTodayTomorrow(g.st, g.def, g.paces, g.at(2, 3)), MoveResult::NothingToMove);   // Thursday rests
    EXPECT_EQ(moveTodayTomorrow(g.st, g.def, g.paces, g.at(2, 1)), MoveResult::Ok);               // Tue reps -> Wed (easy dropped)
    EXPECT_EQ(g.on(g.at(2, 2)).asRun.kind, Kind::Reps);
}

TEST(PlanRules, SkippingLogsSkippedAndAKeySessionTakesALaterQualityLikeAMissedOne)
{
    Fixture f;
    EXPECT_EQ(skipToday(f.st, f.def, f.paces, f.at(5, 1)), MoveResult::Ok);
    EXPECT_EQ(f.st.find(f.at(5, 1))->status, DayStatus::Skipped);
    EXPECT_EQ(f.on(f.at(5, 4)).asRun.kind, Kind::Intervals);
    EXPECT_EQ(skipToday(f.st, f.def, f.paces, f.at(5, 0)), MoveResult::NothingToMove);   // Monday rests
}

TEST(PlanRules, APausedPlanRestsAndMissesNothing)
{
    Fixture f;
    f.run(f.at(0, 1));
    pause(f.st, f.at(0, 2));
    EXPECT_EQ(f.on(f.at(0, 4)).asRun.kind, Kind::Rest);
    const Settled s = settle(f.st, f.def, f.paces, f.at(1, 3));
    EXPECT_EQ(s.missed, 0);
    EXPECT_EQ(f.st.logCount(), 1);
}

TEST(PlanRules, ALongPauseRepeatsTheLastCompletedWeekWithoutARace)
{
    Fixture f;
    const uint8_t weeks = f.st.calendar.weeks;
    pause(f.st, f.at(1, 1));
    uint32_t days = 0;
    EXPECT_EQ(resume(f.st, f.def, f.paces, f.at(2, 4), &days), Resumed::RepeatedWeek);   // 10 days
    EXPECT_EQ(days, 10u);
    EXPECT_EQ(PlanCalendar::planWeekOf(f.st.calendar, 2), 0);   // week 3 runs plan week 1
    EXPECT_EQ(PlanCalendar::planWeekOf(f.st.calendar, 3), 1);   // then plan week 2 on
    EXPECT_EQ(f.st.calendar.weeks, weeks + 2);
    EXPECT_EQ(f.st.pausedFrom, 0u);
}

TEST(PlanRules, AShortPauseJustCarriesOn)
{
    Fixture f;
    pause(f.st, f.at(1, 1));
    EXPECT_EQ(resume(f.st, f.def, f.paces, f.at(1, 6)), Resumed::Resumed);
    EXPECT_EQ(f.st.calendar.adjCount, 0);
    EXPECT_EQ(resume(f.st, f.def, f.paces, f.at(1, 6)), Resumed::NotPaused);
}

TEST(PlanRules, APauseOverFourteenDaysIsEasyRunningUntilATimeTrialThenThePlanResumes)
{
    Fixture f;
    pause(f.st, f.at(1, 1));
    EXPECT_EQ(resume(f.st, f.def, f.paces, f.at(4, 0)), Resumed::Retest);   // 20 days
    EXPECT_EQ(f.on(f.at(4, 1)).asRun.kind, Kind::Easy);
    EXPECT_EQ(f.on(f.at(4, 6)).asRun.kind, Kind::TimeTrial5k);
    // The trial: the plan resumes at the week it was paused in (plan week 2).
    const Recal r = onTrial(f.st, Kind::TimeTrial5k, 0, 22 * 60 + 10, f.at(4, 6));
    EXPECT_EQ(r.action, Recal::Action::Keep);
    EXPECT_EQ(f.st.retestFrom, 0u);
    EXPECT_EQ(PlanCalendar::planWeekOf(f.st.calendar, 5), 1);
    EXPECT_EQ(f.on(f.at(5, 1)).asRun.kind, Kind::Hills);   // plan week 2's Tuesday
}

TEST(PlanRules, Recalibration)
{
    const uint32_t cur = 22 * 60;
    EXPECT_EQ(recalibrate(cur, 5000, 21 * 60 + 20).action, Recal::Action::Apply);    // 3% faster
    EXPECT_EQ(recalibrate(cur, 5000, 22 * 60 + 10).action, Recal::Action::Keep);     // 0.8% slower
    EXPECT_EQ(recalibrate(cur, 5000, 22 * 60 + 50).action, Recal::Action::Ask);      // 3.8% slower
    const Recal j = recalibrate(cur, 5000, 20 * 60 + 30);                            // 6.8% faster
    EXPECT_EQ(j.action, Recal::Action::Ask);
    EXPECT_TRUE(j.jump);
    EXPECT_EQ(recalibrate(cur, 5000, 20 * 60 + 45).action, Recal::Action::Apply);    // 5.7% faster
    EXPECT_EQ(recalibrate(cur, 0, 100).action, Recal::Action::Keep);
    // A 10K gives its 5K equivalent: 45:37 is a 22:00 5K (PLANS.md 2.3).
    const Recal tenK = recalibrate(cur, 10000, 45 * 60 + 37);
    EXPECT_NEAR(static_cast<double>(tenK.fiveKSec), 22.0 * 60.0, 6.0);
}

TEST(PlanRules, ATrialAppliesAsksAndTheAnswerUpdatesOrKeeps)
{
    Fixture f;
    onTrial(f.st, Kind::TimeTrial5k, 0, 21 * 60 + 20, f.at(3, 6));
    EXPECT_EQ(f.st.setup.fiveKSec, 21u * 60 + 20);
    EXPECT_EQ(f.st.setup.fiveKDate, f.at(3, 6));

    Fixture g;
    EXPECT_EQ(onTrial(g.st, Kind::TimeTrial5k, 0, 22 * 60 + 50, g.at(3, 6)).action, Recal::Action::Ask);
    EXPECT_EQ(g.st.setup.fiveKSec, 22u * 60);        // unchanged until answered
    EXPECT_EQ(g.st.askFiveKSec, 22u * 60 + 50);
    answerAsk(g.st, false, g.at(4, 1));
    EXPECT_EQ(g.st.setup.fiveKSec, 22u * 60);
    EXPECT_EQ(g.st.askFiveKSec, 0u);

    Fixture h;
    onTrial(h.st, Kind::TimeTrial5k, 0, 22 * 60 + 50, h.at(3, 6));
    answerAsk(h.st, true, h.at(4, 1));
    EXPECT_EQ(h.st.setup.fiveKSec, 22u * 60 + 50);

    // A session that is not a trial or race does nothing.
    Fixture k;
    EXPECT_EQ(onTrial(k.st, Kind::Easy, 0, 1000, k.at(0, 1)).action, Recal::Action::Keep);
    EXPECT_EQ(k.st.setup.fiveKSec, 22u * 60);
}

TEST(PlanRules, TheMemoryReadsAndWritesAsPlanJson)
{
    Fixture f;
    f.run(f.at(2, 1));
    settle(f.st, f.def, f.paces, f.at(3, 0));                    // a week repeated: adj entries
    skipToday(f.st, f.def, f.paces, f.at(5, 1));                 // an override and a log entry
    doTomorrowToday(f.st, f.def, f.paces, f.at(6, 1));
    pause(f.st, f.at(8, 2));
    f.st.retestFrom = f.at(7, 0);
    f.st.resumeWeek = 3;
    f.st.askFiveKSec = 22 * 60 + 50;
    f.st.offerDeclined = f.at(2, 4);
    std::vector<char> buf(PlanState::kMaxJsonBytes);
    const size_t n = f.st.toJson(buf.data(), buf.size());
    ASSERT_GT(n, 0u);
    PlanState back;
    ASSERT_TRUE(back.fromJson(buf.data(), n));
    EXPECT_EQ(back.calendar.adjCount, f.st.calendar.adjCount);
    EXPECT_EQ(back.calendar.weeks, f.st.calendar.weeks);
    EXPECT_EQ(back.overrideCount, f.st.overrideCount);
    EXPECT_EQ(back.pausedFrom, f.st.pausedFrom);
    EXPECT_EQ(back.retestFrom, f.st.retestFrom);
    EXPECT_EQ(back.resumeWeek, 3);
    EXPECT_EQ(back.settledTo, f.st.settledTo);
    EXPECT_EQ(back.askFiveKSec, 22u * 60 + 50);
    EXPECT_EQ(back.offerDeclined, f.st.offerDeclined);
    // And the same text again.
    std::vector<char> buf2(PlanState::kMaxJsonBytes);
    const size_t n2 = back.toJson(buf2.data(), buf2.size());
    EXPECT_EQ(std::string(buf.data(), n), std::string(buf2.data(), n2));
}

TEST(PlanRules, AFullLogWithTheMemoryStillFitsItsBuffer)
{
    Fixture f;
    for (int i = 0; i < PlanState::kMaxLog; ++i) {
        f.st.record(LogEntry{Date::addDays(20270104u, i), Kind::Easy, DayStatus::Done, 1797314400u + i});
    }
    f.st.calendar.adjCount = kMaxAdjust;
    for (uint8_t i = 0; i < kMaxAdjust; ++i) {
        f.st.calendar.adj[i] = WeekAdjust{static_cast<uint8_t>(i + 1), i % 2 ? int8_t{1} : int8_t{-1}};
    }
    for (int i = 0; i < kMaxOverrides; ++i) {
        f.st.setOverride(Override{Date::addDays(20270601u, i), Date::addDays(20270601u, i + 1), OverrideMode::Session});
    }
    f.st.pausedFrom = 20270601u;
    f.st.retestFrom = 20270601u;
    f.st.resumeWeek = 20;
    f.st.settledTo = 20270601u;
    f.st.askFiveKSec = 3599;
    f.st.offerDeclined = 20270601u;
    std::vector<char> buf(PlanState::kMaxJsonBytes);
    EXPECT_GT(f.st.toJson(buf.data(), buf.size()), 0u);
}

TEST(PlanRules, BadMemoryValuesRefuseTheFile)
{
    const char* head = "{\"v\":1,\"s\":[2,2,86,6,0,0,0,1320,20261115,1,0,0],\"c\":[0,20261214,0,12],\"log\":[]";
    const char* bad[] = {
        ",\"adj\":[[3,20]]}",            // a delta out of range
        ",\"adj\":[[5,9],[2,9]]}",       // out of order
        ",\"ov\":[[20270105,0,1]]}",     // a Session override with no source
        ",\"ov\":[[20270105,0,9]]}",     // an unknown mode
        ",\"ov\":[[20271305,0,2]]}",     // an impossible date
        ",\"ps\":20271305}",
        ",\"ak\":5}",                    // a 5K of 5 seconds
        ",\"rw\":99}",
    };
    for (const char* tail : bad) {
        const std::string t = std::string(head) + tail;
        PlanState s;
        EXPECT_FALSE(s.fromJson(t.data(), t.size())) << t;
    }
}

TEST(PlanRules, GoldenScenarios)
{
    const std::string text = allScenarios();
    const std::string path = std::string(XTRAINING_GOLDEN_DIR) + "/plan_sim.md";
    if (std::getenv("XTRAINING_UPDATE_GOLDEN") != nullptr) {
        std::ofstream(path, std::ios::binary) << text;
        return;
    }
    const std::string want = readFile(path);
    EXPECT_FALSE(want.empty()) << path << " is missing: run with XTRAINING_UPDATE_GOLDEN=1";
    EXPECT_EQ(text, want) << path << " differs: if the change is meant, run with XTRAINING_UPDATE_GOLDEN=1 and read the diff";
}
