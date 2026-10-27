/**
 * Host tests for Streak::LinkReader: the summaries the HybridX apps leave in the
 * shared folder (hybridx-link/docs/ACTIVITY_LINK.md), read the way a watch lets
 * Streak read them: by exact file name, no listing outside the sandbox.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "ActivityLink.hpp"
#include "FitFixture.hpp"
#include "LinkReader.hpp"
#include "StreakModel.hpp"
#include "TreeFileSystem.hpp"
#include "WeekMath.hpp"

using Streak::Found;
using Streak::Kind;
using Streak::LinkReader;

namespace
{

constexpr size_t kCap = 32;

class Seen : public Streak::SeenSet
{
public:
    bool seen(uint32_t appKey, uint32_t localStart) const override
    {
        for (const auto& k : keys) {
            if (k.first == appKey && k.second == localStart) {
                return true;
            }
        }
        return false;
    }
    std::vector<std::pair<uint32_t, uint32_t>> keys;
};

int32_t day(int y, int m, int d)
{
    return Streak::WeekMath::daysFromCivil(y, static_cast<uint32_t>(m), static_cast<uint32_t>(d));
}

// Tuesday 27 October 2026: the week started Monday the 26th.
const int32_t kToday = day(2026, 10, 27);
const int32_t kFrom  = day(2026, 10, 19);   // the previous week and this one

/// An activity that started at a local time, in a zone (minutes east of UTC).
HybridXLink::Entry entry(int y, int mo, int d, int h, int mi, int zoneMin = 60, uint32_t seconds = 2700,
                         HybridXLink::Sport sport = HybridXLink::Sport::Run)
{
    HybridXLink::Entry e;
    e.startUtc    = Fixture::unixOf(y, mo, d, h, mi, 0, zoneMin);
    e.tzOffsetMin = static_cast<int16_t>(zoneMin);
    e.durationSec = seconds;
    e.distanceM   = 5000;
    e.avgHr       = 150;
    e.maxHr       = 170;
    e.sport       = sport;
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

constexpr const char* kShared = "/Apps/SharedData/HybridX/";

/// A watch: the app in its sandbox, firmware that confines it, a file per app.
struct Watch {
    TreeFileSystem fs {"/Apps/HybridXStreak"};
    Watch()
    {
        fs.addDir("/Apps/SharedData/HybridX");
        fs.blockParentAccess(true);
        fs.blockListingOutside(true);
    }
    void leave(const char* slotFile, const char* app, const std::vector<HybridXLink::Entry>& entries)
    {
        fs.addFile(std::string(kShared) + slotFile, fileOf(app, entries));
    }
};

size_t read(LinkReader& r, Watch& w, const Seen& seen, std::vector<Found>& found, size_t count = 0, size_t cap = kCap)
{
    found.resize(cap);
    return r.addTo(w.fs, kFrom, kToday, seen, found.data(), count, cap);
}

} // namespace

TEST(LinkReader, NothingLeftMeansNothingFound)
{
    Watch w;
    LinkReader r;
    std::vector<Found> found;
    EXPECT_EQ(read(r, w, Seen(), found), 0u);
    EXPECT_EQ(r.stats().files, 0);
}

TEST(LinkReader, ARunBecomesAFoundActivity)
{
    Watch w;
    w.leave("act_run.json", "HybridXRun", {entry(2026, 10, 27, 7, 30)});
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, Seen(), found), 1u);

    const Found& f = found[0];
    EXPECT_EQ(f.appKey, Streak::ActivityScanner::appKey("HybridXRun"));
    EXPECT_STREQ(f.app, "HybridXRun");
    EXPECT_EQ(f.kind, Kind::Run);
    EXPECT_EQ(f.minutes, 45);   // 2700 s
    EXPECT_EQ(f.localDay, kToday);
    EXPECT_EQ(f.localStart, static_cast<uint32_t>(kToday) * 86400u + 7u * 3600u + 30u * 60u);
    EXPECT_EQ(r.stats().files, 1);
    EXPECT_EQ(r.stats().added, 1);
}

TEST(LinkReader, MinutesAreWholeMinutesLikeTheFitReader)
{
    Watch w;
    w.leave("act_run.json", "HybridXRun",
            {entry(2026, 10, 27, 7, 0, 60, 599), entry(2026, 10, 27, 8, 0, 60, 600), entry(2026, 10, 27, 9, 0, 60, 659)});
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, Seen(), found), 3u);
    EXPECT_EQ(found[0].minutes, 9);
    EXPECT_EQ(found[1].minutes, 10);
    EXPECT_EQ(found[2].minutes, 10);
}

TEST(LinkReader, TheLocalDayFollowsTheZone)
{
    // 00:30 on the 27th in New Zealand (UTC+13) is 11:30 on the 26th in UTC. It is the 27th for the athlete.
    Watch w;
    w.leave("act_run.json", "HybridXRun", {entry(2026, 10, 27, 0, 30, 13 * 60)});
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, Seen(), found), 1u);
    EXPECT_EQ(found[0].localDay, kToday);
    EXPECT_EQ(found[0].localStart % 86400u, 30u * 60u);

    // And 23:30 on the 26th in California (UTC-7) is the 27th in UTC: still the 26th for the athlete.
    Watch w2;
    w2.leave("act_run.json", "HybridXRun", {entry(2026, 10, 26, 23, 30, -7 * 60)});
    std::vector<Found> found2;
    ASSERT_EQ(read(r, w2, Seen(), found2), 1u);
    EXPECT_EQ(found2[0].localDay, day(2026, 10, 26));
}

TEST(LinkReader, OnlyTheScanWindowIsRead)
{
    Watch w;
    w.leave("act_run.json", "HybridXRun",
            {entry(2026, 10, 12, 8, 0),    // two weeks ago: before the window
             entry(2026, 10, 19, 8, 0),    // the first day of the window
             entry(2026, 10, 27, 8, 0),    // today
             entry(2026, 10, 28, 8, 0)});  // tomorrow: a clock set back, left for later
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, Seen(), found), 2u);
    EXPECT_EQ(found[0].localDay, day(2026, 10, 19));
    EXPECT_EQ(found[1].localDay, kToday);
    EXPECT_EQ(r.stats().outside, 2);
}

TEST(LinkReader, WhatWasCountedIsNotAddedAgain)
{
    Watch w;
    const auto e = entry(2026, 10, 27, 7, 30);
    w.leave("act_run.json", "HybridXRun", {e, entry(2026, 10, 26, 7, 30)});
    Seen seen;
    seen.keys.emplace_back(Streak::ActivityScanner::appKey("HybridXRun"), e.localStart());
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, seen, found), 1u);
    EXPECT_EQ(found[0].localDay, day(2026, 10, 26));
    EXPECT_EQ(r.stats().counted, 1);
}

TEST(LinkReader, EveryAppsFileIsReadAndTheListIsOldestFirst)
{
    Watch w;
    w.leave("act_run.json", "HybridXRun", {entry(2026, 10, 27, 7, 0)});
    w.leave("act_race.json", "HybridXRace", {entry(2026, 10, 25, 9, 0, 60, 3600, HybridXLink::Sport::Hybrid)});
    w.leave("act_trail.json", "HybridXTrail", {entry(2026, 10, 26, 6, 0, 60, 5400)});
    // A bike app and a workout app that do not exist yet: their slots are reserved, so Streak needs no change.
    w.leave("act_ride.json", "HybridXRide", {entry(2026, 10, 24, 17, 0, 60, 3000, HybridXLink::Sport::Ride)});
    w.leave("act_workout.json", "HybridXWorkout", {entry(2026, 10, 27, 18, 0, 60, 2400, HybridXLink::Sport::Strength)});
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, Seen(), found), 5u);
    EXPECT_EQ(r.stats().files, 5);

    for (size_t i = 1; i < 5; ++i) {
        EXPECT_LE(found[i - 1].localStart, found[i].localStart) << i;
    }
    EXPECT_STREQ(found[0].app, "HybridXRide");
    EXPECT_EQ(found[0].kind, Kind::Ride);
    EXPECT_STREQ(found[1].app, "HybridXRace");
    EXPECT_EQ(found[1].kind, Kind::Hybrid);
    EXPECT_STREQ(found[2].app, "HybridXTrail");
    EXPECT_EQ(found[3].kind, Kind::Run);
    EXPECT_EQ(found[4].kind, Kind::Strength);
}

TEST(LinkReader, AFileOutsideTheTableIsNotRead)
{
    Watch w;
    w.leave("act_unknownapp.json", "HybridXOther", {entry(2026, 10, 27, 7, 0)});
    w.leave("act_run.json.tmp", "HybridXRun", {entry(2026, 10, 27, 8, 0)});
    LinkReader r;
    std::vector<Found> found;
    EXPECT_EQ(read(r, w, Seen(), found), 0u);
}

TEST(LinkReader, ABrokenPrimaryFallsBackToTheBackup)
{
    Watch w;
    w.fs.addFile(std::string(kShared) + "act_run.json", "{\"v\":1,\"app\":\"HybridXRun\",\"a\":[[17");   // cut short
    w.fs.addFile(std::string(kShared) + "act_run.json.bak", fileOf("HybridXRun", {entry(2026, 10, 27, 7, 0)}));
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, Seen(), found), 1u);
    EXPECT_EQ(r.stats().files, 1);
}

TEST(LinkReader, AFileThatDoesNotReadIsSkippedAndTheOthersStillAre)
{
    Watch w;
    w.fs.addFile(std::string(kShared) + "act_run.json", "not json at all");
    w.leave("act_race.json", "HybridXRace", {entry(2026, 10, 27, 7, 0, 60, 3600, HybridXLink::Sport::Hybrid)});
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, Seen(), found), 1u);
    EXPECT_EQ(r.stats().files, 1);
}

TEST(LinkReader, ABadEntryIsSkippedAndTheRestKept)
{
    Watch w;
    w.fs.addFile(std::string(kShared) + "act_run.json",
                 R"({"v":1,"app":"HybridXRun","a":[[1500000000,1500,2700,5400,152,171,1],)"   // before 2020
                 R"([1793100000,1500,2700,5400,152,171,1]]})");
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, Seen(), found), 1u);
    EXPECT_EQ(r.stats().invalid, 1);
    EXPECT_EQ(r.stats().entries, 2);
}

TEST(LinkReader, ItAddsToWhatTheScannerFoundKeepingTheOrder)
{
    Watch w;
    w.leave("act_run.json", "HybridXRun", {entry(2026, 10, 22, 7, 0), entry(2026, 10, 27, 7, 0)});

    std::vector<Found> found(kCap);
    Found early;
    early.appKey     = Streak::ActivityScanner::appKey("Walking");
    early.localDay   = day(2026, 10, 20);
    early.localStart = static_cast<uint32_t>(early.localDay) * 86400u + 9u * 3600u;
    Found mid = early;
    mid.localDay   = day(2026, 10, 25);
    mid.localStart = static_cast<uint32_t>(mid.localDay) * 86400u + 9u * 3600u;
    found[0] = early;
    found[1] = mid;

    LinkReader r;
    const size_t n = r.addTo(w.fs, kFrom, kToday, Seen(), found.data(), 2, kCap);
    ASSERT_EQ(n, 4u);
    EXPECT_EQ(found[0].localDay, day(2026, 10, 20));
    EXPECT_EQ(found[1].localDay, day(2026, 10, 22));
    EXPECT_EQ(found[2].localDay, day(2026, 10, 25));
    EXPECT_EQ(found[3].localDay, day(2026, 10, 27));
}

TEST(LinkReader, WhenTheListIsFullTheNewestWaitForTheNextOpen)
{
    Watch w;
    w.leave("act_run.json", "HybridXRun",
            {entry(2026, 10, 20, 7, 0), entry(2026, 10, 21, 7, 0), entry(2026, 10, 22, 7, 0), entry(2026, 10, 23, 7, 0),
             entry(2026, 10, 24, 7, 0)});
    LinkReader r;
    std::vector<Found> found;
    const size_t n = read(r, w, Seen(), found, 0, 3);   // room for three
    ASSERT_EQ(n, 3u);
    EXPECT_EQ(found[0].localDay, day(2026, 10, 20));
    EXPECT_EQ(found[1].localDay, day(2026, 10, 21));
    EXPECT_EQ(found[2].localDay, day(2026, 10, 22));
    EXPECT_EQ(r.stats().deferred, 2);
}

// ---- with the model: the week counts them, once ------------------------------------------

namespace
{

Streak::Events openOn(Streak::StreakModel& m, LinkReader& r, Watch& w, Found* found, size_t count)
{
    int32_t from = 0, to = 0;
    m.scanWindow(kToday, from, to);
    const size_t n = r.addTo(w.fs, from, to, m, found, count, kCap);
    Streak::Events ev;
    m.update(kToday, found, n, ev);
    return ev;
}

} // namespace

TEST(LinkReaderWithModel, TheSummariesCountTowardTheWeek)
{
    Watch w;
    w.leave("act_run.json", "HybridXRun", {entry(2026, 10, 26, 7, 0)});
    w.leave("act_ride.json", "HybridXRide", {entry(2026, 10, 27, 7, 0, 60, 3000, HybridXLink::Sport::Ride)});

    Streak::StreakModel m;
    m.reset(Streak::Goal {});
    LinkReader r;
    Found found[kCap];
    openOn(m, r, w, found, 0);
    EXPECT_EQ(m.qualifying(), 2);
}

TEST(LinkReaderWithModel, OpeningAgainDoesNotCountThemAgain)
{
    Watch w;
    w.leave("act_run.json", "HybridXRun", {entry(2026, 10, 26, 7, 0), entry(2026, 10, 27, 7, 0)});

    Streak::StreakModel m;
    m.reset(Streak::Goal {});
    LinkReader r;
    Found found[kCap];
    openOn(m, r, w, found, 0);
    ASSERT_EQ(m.qualifying(), 2);
    openOn(m, r, w, found, 0);
    EXPECT_EQ(m.qualifying(), 2);
    EXPECT_EQ(r.stats().counted, 2);
    EXPECT_EQ(r.stats().added, 0);
}

TEST(LinkReaderWithModel, AShortSessionDoesNotCountButADayLaterOneDoes)
{
    Watch w;
    w.leave("act_run.json", "HybridXRun", {entry(2026, 10, 26, 7, 0, 60, 300), entry(2026, 10, 27, 7, 0, 60, 1500)});

    Streak::StreakModel m;
    m.reset(Streak::Goal {});   // at least 10 minutes counts
    LinkReader r;
    Found found[kCap];
    openOn(m, r, w, found, 0);
    EXPECT_EQ(m.qualifying(), 1);
}

TEST(LinkReaderWithModel, TheSameRunSeenByTheScannerAndTheSummaryCountsOnce)
{
    // The simulator lets Streak read the app's .fit as well as its summary; one run, one count.
    Watch w;
    w.leave("act_run.json", "HybridXRun", {entry(2026, 10, 27, 7, 30)});

    Found found[kCap];
    Found& scanned     = found[0];
    scanned.appKey     = Streak::ActivityScanner::appKey("HybridXRun");
    scanned.localStart = entry(2026, 10, 27, 7, 30).localStart();
    scanned.localDay   = kToday;
    scanned.kind       = Kind::Run;
    scanned.minutes    = 45;
    std::strncpy(scanned.app, "HybridXRun", sizeof(scanned.app) - 1);

    Streak::StreakModel m;
    m.reset(Streak::Goal {});
    LinkReader r;
    openOn(m, r, w, found, 1);
    EXPECT_EQ(r.stats().added, 1);   // both are in the list...
    EXPECT_EQ(m.qualifying(), 1);    // ...and the model counts the first and skips the second
}

TEST(LinkReaderWithModel, ARunTheRealAppWroteIsFound)
{
    // The bytes the Run app's writer made in the simulator (hybridx-run NOTES, "Activity link").
    Watch w;
    w.fs.addFile(std::string(kShared) + "act_run.json",
                 R"({"v":1,"app":"HybridXRun","a":[[1793099301,1440,2700,215,144,160,1]]})");
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, Seen(), found), 1u);
    EXPECT_EQ(found[0].localDay, kToday);                                    // 11:08:21 UTC on the 27th, zone 0
    EXPECT_EQ(found[0].localStart % 86400u, 11u * 3600u + 8u * 60u + 21u);
    EXPECT_EQ(found[0].minutes, 45);
    EXPECT_STREQ(found[0].app, "HybridXRun");
}

TEST(LinkReaderWithModel, ARaceTheRealAppWroteIsFoundAsHybrid)
{
    // The bytes the Race app's writer made in the simulator (hybridx-race NOTES, "Activity link").
    Watch w;
    w.fs.addFile(std::string(kShared) + "act_race.json",
                 R"({"v":1,"app":"HybridXRace","a":[[1793103288,1440,50,5180,150,157,6],[1793103347,1440,50,5180,141,145,6]]})");
    LinkReader r;
    std::vector<Found> found;
    ASSERT_EQ(read(r, w, Seen(), found), 2u);
    EXPECT_STREQ(found[0].app, "HybridXRace");
    EXPECT_EQ(found[0].appKey, Streak::ActivityScanner::appKey("HybridXRace"));
    EXPECT_EQ(found[0].kind, Kind::Hybrid);
    EXPECT_EQ(found[0].localDay, kToday);
    EXPECT_EQ(found[0].localStart % 86400u, 12u * 3600u + 14u * 60u + 48u);
    EXPECT_LT(found[0].localStart, found[1].localStart);
}

