/**
 * Host tests for the classifier (PLAN 5.3) and the activity scanner (PLAN 5.1),
 * over TreeFileSystem with FIT files from the SDK's own FitWriter.
 */

#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <utility>

#include "ActivityScanner.hpp"
#include "Classifier.hpp"
#include "FitFixture.hpp"
#include "TreeFileSystem.hpp"
#include "WeekMath.hpp"

using Streak::ActivityScanner;
using Streak::Found;
using Streak::Kind;

namespace
{

constexpr const char* kOwn = "HybridXStreak";

class Seen : public Streak::SeenSet
{
public:
    bool seen(uint32_t appKey, uint32_t localStart) const override { return keys.count({appKey, localStart}) > 0; }
    std::set<std::pair<uint32_t, uint32_t>> keys;
};

int32_t day(int y, int m, int d)
{
    return Streak::WeekMath::daysFromCivil(y, static_cast<uint32_t>(m), static_cast<uint32_t>(d));
}

/// Add an activity the way an SDK app writes it: local-time name, UTC session start.
void addActivity(TreeFileSystem& fs, const std::string& app, int y, int mo, int d, int h, int mi, uint8_t sport,
                 uint32_t minutes, int offsetMin = 60, uint8_t sub = 0)
{
    Fixture::FitSpec spec;
    spec.sport     = sport;
    spec.subSport  = sub;
    spec.timerS    = minutes * 60u;
    spec.startUnix = Fixture::unixOf(y, mo, d, h, mi, 0, offsetMin);
    char month[8];
    snprintf(month, sizeof(month), "%04d%02d", y, mo);
    fs.addFile("/Apps/" + app + "/Activity/" + month + "/" + Fixture::activityName(y, mo, d, h, mi, 0),
               Fixture::fitBytes(spec));
}

} // namespace

TEST(Classifier, EveryRow)
{
    // FitProfile Sport: Generic 0, Running 1, Cycling 2, Training 10, Walking 11, Hiking 17.
    EXPECT_EQ(Streak::classify(1, 0, "Running"), Kind::Run);
    EXPECT_EQ(Streak::classify(1, 1, "Treadmill"), Kind::Run);
    EXPECT_EQ(Streak::classify(2, 0, "Cycling"), Kind::Ride);
    EXPECT_EQ(Streak::classify(11, 0, "Hiking"), Kind::Walk);
    EXPECT_EQ(Streak::classify(17, 0, "Hiking"), Kind::Walk);
    EXPECT_EQ(Streak::classify(10, 26, "HybridXRace"), Kind::Hybrid);   // the folder beats the sport
    EXPECT_EQ(Streak::classify(1, 0, "HybridXRace"), Kind::Hybrid);     // Race writes Running/Generic (D2)
    EXPECT_EQ(Streak::classify(10, 0, "Strength"), Kind::Strength);
    EXPECT_EQ(Streak::classify(0, 0, "Workout"), Kind::Workout);
    EXPECT_EQ(Streak::classify(0, 0, "HRMonitor"), Kind::Other);
    EXPECT_EQ(Streak::classify(53, 0, "Somebody"), Kind::Other);        // outside the SDK enum
    EXPECT_EQ(Streak::classify(0xFF, 0xFF, nullptr), Kind::Other);
}

TEST(ActivityScanner, ParsesTheSdkName)
{
    uint32_t t = 0;
    ASSERT_TRUE(ActivityScanner::parseName("activity_20260923T181000.fit", t));
    EXPECT_EQ(t, static_cast<uint32_t>(day(2026, 9, 23)) * 86400u + 18u * 3600u + 10u * 60u);
    EXPECT_TRUE(ActivityScanner::parseName("activity_20260923T181000.FIT", t));
    EXPECT_FALSE(ActivityScanner::parseName("activity_20260923-181000.fit", t));
    EXPECT_FALSE(ActivityScanner::parseName("activity_20261323T181000.fit", t));
    EXPECT_FALSE(ActivityScanner::parseName("run_20260923T181000.fit", t));
    EXPECT_FALSE(ActivityScanner::parseName(".recording", t));
}

TEST(ActivityScanner, CivilFromDaysInvertsDaysFromCivil)
{
    for (int32_t d = day(2023, 12, 1); d < day(2029, 3, 1); ++d) {
        const auto c = Streak::WeekMath::civilFromDays(d);
        ASSERT_EQ(Streak::WeekMath::daysFromCivil(c.year, c.month, c.day), d);
    }
}

TEST(ActivityScanner, FindsEveryAppsActivitiesOldestFirst)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    addActivity(fs, "Running", 2026, 9, 23, 18, 10, 1, 42);
    addActivity(fs, "Cycling", 2026, 9, 21, 7, 0, 2, 64);
    addActivity(fs, "HybridXRace", 2026, 9, 22, 12, 0, 1, 71);
    addActivity(fs, "Workout", 2026, 9, 24, 6, 30, 0, 45);
    addActivity(fs, "HybridXStreak", 2026, 9, 22, 9, 0, 1, 30);   // our own: never scanned
    fs.addDir("/Apps/SharedData/HybridX");
    fs.addDir("/Apps/Settings");

    ActivityScanner scanner;
    Seen            seen;
    Found           out[8];
    const size_t n = scanner.scan(fs, kOwn, day(2026, 9, 14), day(2026, 9, 27), seen, out, 8);

    ASSERT_EQ(n, 4u);
    EXPECT_STREQ(out[0].app, "Cycling");
    EXPECT_EQ(out[0].kind, Kind::Ride);
    EXPECT_EQ(out[0].minutes, 64);
    EXPECT_STREQ(out[1].app, "HybridXRace");
    EXPECT_EQ(out[1].kind, Kind::Hybrid);
    EXPECT_STREQ(out[2].app, "Running");
    EXPECT_EQ(out[2].kind, Kind::Run);
    EXPECT_EQ(out[2].localDay, day(2026, 9, 23));
    EXPECT_EQ(out[2].localStart % 86400u, 18u * 3600u + 600u);
    EXPECT_STREQ(out[3].app, "Workout");
    EXPECT_EQ(out[3].kind, Kind::Workout);
    EXPECT_EQ(out[2].appKey, ActivityScanner::appKey("Running"));
    EXPECT_TRUE(scanner.stats().listed);
    EXPECT_EQ(scanner.stats().apps, 5u);   // Cycling, HybridXRace, Running, Settings, Workout
}

TEST(ActivityScanner, SkipsSeenFilesAndTheWindowEdges)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    addActivity(fs, "Running", 2026, 9, 13, 8, 0, 1, 30);    // before the window
    addActivity(fs, "Running", 2026, 9, 14, 8, 0, 1, 30);    // first day
    addActivity(fs, "Running", 2026, 9, 27, 21, 0, 1, 30);   // last day
    addActivity(fs, "Running", 2026, 9, 28, 8, 0, 1, 30);    // after it
    ActivityScanner scanner;
    Seen            seen;
    uint32_t        t = 0;
    ActivityScanner::parseName("activity_20260914T080000.fit", t);
    seen.keys.insert({ActivityScanner::appKey("Running"), t});

    Found        out[8];
    const size_t n = scanner.scan(fs, kOwn, day(2026, 9, 14), day(2026, 9, 27), seen, out, 8);
    ASSERT_EQ(n, 1u);
    EXPECT_EQ(out[0].localDay, day(2026, 9, 27));
}

TEST(ActivityScanner, WindowAcrossAMonthEndListsBothMonths)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    addActivity(fs, "Running", 2026, 8, 30, 8, 0, 1, 30);
    addActivity(fs, "Running", 2026, 9, 2, 8, 0, 1, 30);
    ActivityScanner scanner;
    Seen            seen;
    Found           out[8];
    EXPECT_EQ(scanner.scan(fs, kOwn, day(2026, 8, 24), day(2026, 9, 6), seen, out, 8), 2u);
    // And across a year end.
    addActivity(fs, "Running", 2026, 12, 31, 8, 0, 1, 30);
    addActivity(fs, "Running", 2027, 1, 1, 8, 0, 1, 30);
    EXPECT_EQ(scanner.scan(fs, kOwn, day(2026, 12, 28), day(2027, 1, 10), seen, out, 8), 2u);
}

TEST(ActivityScanner, SkipsTheFileStillBeingRecorded)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    addActivity(fs, "Hiking", 2026, 9, 22, 9, 0, 17, 90);
    addActivity(fs, "Hiking", 2026, 9, 24, 9, 0, 17, 90);
    // RecordingMarker: the .fit path on line 1, the offset on line 2.
    fs.addFile("/Apps/Hiking/Activity/.recording", "Activity/202609/activity_20260924T090000.fit\n4096\n");
    ActivityScanner scanner;
    Seen            seen;
    Found           out[8];
    ASSERT_EQ(scanner.scan(fs, kOwn, day(2026, 9, 21), day(2026, 9, 27), seen, out, 8), 1u);
    EXPECT_EQ(out[0].localDay, day(2026, 9, 22));
    EXPECT_EQ(scanner.stats().recording, 1u);
}

TEST(ActivityScanner, BrokenFilesAreNotReportedSoTheyAreRetried)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    addActivity(fs, "Running", 2026, 9, 22, 9, 0, 1, 30);
    fs.addFile("/Apps/Running/Activity/202609/activity_20260923T090000.fit", "half a file");
    ActivityScanner scanner;
    Seen            seen;
    Found           out[8];
    EXPECT_EQ(scanner.scan(fs, kOwn, day(2026, 9, 21), day(2026, 9, 27), seen, out, 8), 1u);
    EXPECT_EQ(scanner.stats().rejected, 1u);
}

TEST(ActivityScanner, NameAndSessionStartMustAgree)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    // Named 23 September, but the session says three days earlier.
    Fixture::FitSpec spec;
    spec.startUnix = Fixture::unixOf(2026, 9, 20, 9, 0, 0);
    fs.addFile("/Apps/Running/Activity/202609/activity_20260923T090000.fit", Fixture::fitBytes(spec));
    // A zone far from UTC is fine: 13 hours ahead.
    addActivity(fs, "Running", 2026, 9, 24, 9, 0, 1, 30, 13 * 60);
    ActivityScanner scanner;
    Seen            seen;
    Found           out[8];
    ASSERT_EQ(scanner.scan(fs, kOwn, day(2026, 9, 21), day(2026, 9, 27), seen, out, 8), 1u);
    EXPECT_EQ(out[0].localDay, day(2026, 9, 24));
    EXPECT_EQ(scanner.stats().mismatched, 1u);
}

TEST(ActivityScanner, ReadsAtMostKMaxNewAndDefersTheRest)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    for (int i = 0; i < 40; ++i) {
        addActivity(fs, "Running", 2026, 9, 22 + i / 20, 6 + (i % 20) / 2, (i % 2) * 30, 1, 20);
    }
    ActivityScanner scanner;
    Seen            seen;
    Found           out[64];
    const size_t    n = scanner.scan(fs, kOwn, day(2026, 9, 21), day(2026, 9, 27), seen, out, 64);
    EXPECT_EQ(n, ActivityScanner::kMaxNew);
    EXPECT_EQ(scanner.stats().deferred, 40u - ActivityScanner::kMaxNew);
    // Oldest first: the 22nd's all came before the 23rd's.
    EXPECT_EQ(out[0].localDay, day(2026, 9, 22));
    for (size_t i = 1; i < n; ++i) {
        EXPECT_LE(out[i - 1].localStart, out[i].localStart);
    }
}

TEST(ActivityScanner, NothingWhenTheWatchKeepsAppsApart)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    addActivity(fs, "Running", 2026, 9, 22, 9, 0, 1, 30);
    fs.blockParentAccess(true);
    ActivityScanner scanner;
    Seen            seen;
    Found           out[8];
    EXPECT_EQ(scanner.scan(fs, kOwn, day(2026, 9, 21), day(2026, 9, 27), seen, out, 8), 0u);
    EXPECT_FALSE(scanner.stats().listed);
}

TEST(ActivityScanner, RealRaceFileIsAHybridSession)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    std::ifstream  in(std::string(RACE_FIT_DIR) + "/K-sim-500m-runs.fit", std::ios::binary);
    std::string    bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    // fitdecode: start 1159001336 FIT s = 2026-09-22 08:48:56 UTC. Name it in BST.
    fs.addFile("/Apps/HybridXRace/Activity/202609/activity_20260922T094856.fit", bytes);
    ActivityScanner scanner;
    Seen            seen;
    Found           out[4];
    ASSERT_EQ(scanner.scan(fs, kOwn, day(2026, 9, 21), day(2026, 9, 27), seen, out, 4), 1u);
    EXPECT_EQ(out[0].kind, Kind::Hybrid);
    EXPECT_EQ(out[0].minutes, 4144 / 60);
}
