/**
 * Host tests for Streak::WeekTotals: the week by sport (NOTES "S7"), summed from the
 * X family's summaries as LinkReader reads them.
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ActivityLink.hpp"
#include "FitFixture.hpp"
#include "LinkReader.hpp"
#include "StreakModel.hpp"
#include "TreeFileSystem.hpp"
#include "WeekMath.hpp"
#include "WeekTotals.hpp"

using Streak::Kind;
using Streak::WeekSum;
using Streak::WeekTotals;

namespace
{

int32_t day(int y, int m, int d)
{
    return Streak::WeekMath::daysFromCivil(y, static_cast<uint32_t>(m), static_cast<uint32_t>(d));
}

// The week of Monday 16 November 2026, and the one before it.
const int32_t kMon     = day(2026, 11, 16);
const int32_t kSun     = kMon + 6;
const int32_t kLastMon = kMon - 7;

class NoneSeen : public Streak::SeenSet
{
public:
    bool seen(uint32_t, uint32_t) const override { return false; }
};

/// Every key seen: what the model reports once a week's activities are counted.
class AllSeen : public Streak::SeenSet
{
public:
    bool seen(uint32_t, uint32_t) const override { return true; }
};

HybridXLink::Entry entry(int y, int mo, int d, int h, uint32_t seconds, uint32_t metres, HybridXLink::Sport sport)
{
    HybridXLink::Entry e;
    e.startUtc    = Fixture::unixOf(y, mo, d, h, 0, 0, 0);
    e.tzOffsetMin = 0;
    e.durationSec = seconds;
    e.distanceM   = metres;
    e.avgHr       = 140;
    e.maxHr       = 170;
    e.sport       = sport;
    e.load        = 30;
    return e;
}

std::string fileOf(const char* app, const std::vector<HybridXLink::Entry>& entries)
{
    std::string text;
    for (const auto& e : entries) {
        char buf[HybridXLink::kMaxBytes];
        const size_t len = HybridXLink::append(text.empty() ? nullptr : text.c_str(), text.size(), app, e, buf, sizeof(buf));
        EXPECT_GT(len, 0u);
        text.assign(buf, len);
    }
    return text;
}

} // namespace

TEST(WeekTotals, SumsByKindAndInAll)
{
    WeekTotals t;
    t.onActivity(kMon, Kind::Run, 1800, 5000, 20);
    t.onActivity(kMon + 2, Kind::Run, 2100, 6200, 25);
    t.onActivity(kMon + 3, Kind::Ride, 3600, 25000, 40);
    t.onActivity(kMon + 4, Kind::Strength, 2700, 0, 30);

    const WeekSum w = t.sum(kMon, kSun);
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Run)].sessions, 2);
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Run)].minutes, 65);   // 3900 s
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Run)].metres, 11200u);
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Ride)].metres, 25000u);
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Strength)].sessions, 1);
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Strength)].metres, 0u);
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Walk)].sessions, 0);
    EXPECT_EQ(w.all.sessions, 4);
    EXPECT_EQ(w.all.minutes, 170);   // 10200 s
    EXPECT_EQ(w.all.metres, 36200u);
}

TEST(WeekTotals, SecondsAreSummedBeforeTheyAreMinutes)
{
    // Ten sessions of 59 s: 590 s is 9 minutes, where minutes per session would make 0.
    WeekTotals t;
    for (int i = 0; i < 10; ++i) {
        t.onActivity(kMon, Kind::Workout, 59, 0, 0);
    }
    EXPECT_EQ(t.sum(kMon, kSun).all.minutes, 9);
}

TEST(WeekTotals, OnlyTheDaysAskedFor)
{
    WeekTotals t;
    t.onActivity(kLastMon, Kind::Run, 600, 2000, 0);     // last week's first day
    t.onActivity(kMon - 1, Kind::Run, 1200, 3000, 0);    // last week's last day
    t.onActivity(kMon, Kind::Run, 1800, 4000, 0);        // this week's first day
    t.onActivity(kSun, Kind::Run, 2400, 5000, 0);        // this week's last day
    t.onActivity(kSun + 1, Kind::Run, 3000, 6000, 0);    // next week: not either

    const WeekSum now  = t.sum(kMon, kSun);
    const WeekSum last = t.sum(kLastMon, kMon - 1);
    EXPECT_EQ(now.all.sessions, 2);
    EXPECT_EQ(now.all.metres, 9000u);
    EXPECT_EQ(last.all.sessions, 2);
    EXPECT_EQ(last.all.minutes, 30);
    EXPECT_EQ(last.all.metres, 5000u);
}

TEST(WeekTotals, FullListDropsAndCounts)
{
    WeekTotals t;
    for (uint16_t i = 0; i < WeekTotals::kMax + 3; ++i) {
        t.onActivity(kMon, Kind::Run, 60, 100, 0);
    }
    EXPECT_EQ(t.count(), WeekTotals::kMax);
    EXPECT_EQ(t.dropped(), 3);
    EXPECT_EQ(t.sum(kMon, kSun).all.sessions, WeekTotals::kMax);
    t.clear();
    EXPECT_EQ(t.count(), 0);
    EXPECT_EQ(t.sum(kMon, kSun).all.sessions, 0);
}

TEST(WeekTotals, AKindOutOfRangeIsOther)
{
    WeekTotals t;
    t.onActivity(kMon, static_cast<Kind>(200), 600, 0, 0);
    EXPECT_EQ(t.sum(kMon, kSun).kind[static_cast<int>(Kind::Other)].sessions, 1);
}

TEST(WeekTotals, LargeSumsAreHeldToTheirFields)
{
    WeekTotals t;
    for (int i = 0; i < 30; ++i) {
        t.onActivity(kMon, Kind::Ride, 48u * 3600u, 1000000u, 0);   // the link's largest: 48 h, 1000 km
    }
    const WeekSum w = t.sum(kMon, kSun);
    EXPECT_EQ(w.all.minutes, 0xFFFF);         // 86400 minutes, held to 16 bits
    EXPECT_EQ(w.all.metres, 30000000u);       // fits 32 bits
}

TEST(WeekTotals, HearsEveryEntryLinkReaderReads)
{
    // Two apps' summaries on a watch. LinkReader adds only the new ones in its window to the
    // list; the totals hear every valid entry, counted or not, in the window or not.
    TreeFileSystem fs {"/Apps/HybridXStreak"};
    fs.addDir("/Apps/SharedData/HybridX");
    fs.blockParentAccess(true);
    fs.blockListingOutside(true);
    fs.addFile("/Apps/SharedData/HybridX/act_run.json",
               fileOf("HybridXRun", {entry(2026, 11, 10, 7, 1800, 5000, HybridXLink::Sport::Run),
                                     entry(2026, 11, 17, 7, 2400, 7000, HybridXLink::Sport::Run),
                                     entry(2026, 11, 19, 7, 3000, 9000, HybridXLink::Sport::Run)}));
    fs.addFile("/Apps/SharedData/HybridX/act_ride.json",
               fileOf("HybridXRide", {entry(2026, 11, 18, 17, 3600, 30000, HybridXLink::Sport::Ride)}));
    fs.addFile("/Apps/SharedData/HybridX/act_workout.json",
               fileOf("HybridXWorkout", {entry(2026, 11, 20, 18, 2700, 0, HybridXLink::Sport::Strength)}));

    Streak::LinkReader reader;
    WeekTotals         totals;
    Streak::Found      found[16];
    AllSeen            seen;   // every one already counted: none is added, all are heard
    const size_t       n = reader.addTo(fs, kMon, kSun, seen, found, 0, 16, &totals);
    EXPECT_EQ(n, 0u);
    EXPECT_EQ(totals.count(), 5);

    const WeekSum w = totals.sum(kMon, kSun);
    EXPECT_EQ(w.all.sessions, 4);
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Run)].sessions, 2);
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Run)].metres, 16000u);
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Ride)].minutes, 60);
    EXPECT_EQ(w.kind[static_cast<int>(Kind::Strength)].sessions, 1);
    EXPECT_EQ(totals.sum(kLastMon, kMon - 1).all.sessions, 1);   // the 10th, outside the window

    // Without a sink nothing changes for the list.
    NoneSeen none;
    EXPECT_EQ(reader.addTo(fs, kMon, kSun, none, found, 0, 16), 4u);
}
