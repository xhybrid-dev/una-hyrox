/**
 * Host tests for the Streak Probe's checks (Tools/Probe), against the
 * TreeFileSystem fake: a watch where apps can see each other, one where the
 * firmware keeps them apart, and the cases in between.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "ProbeRunner.hpp"
#include "TreeFileSystem.hpp"

using Probe::Check;
using Probe::Verdict;

namespace
{

constexpr const char* kSandbox = "/Apps/HXStreakProbe";

class FakeHost : public Probe::Host
{
public:
    void line(const char* text) override { lines.emplace_back(text); }
    uint32_t nowMs() override { return now += 7u; }

    bool said(const std::string& fragment) const
    {
        for (const auto& l : lines) {
            if (l.find(fragment) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    std::vector<std::string> lines;
    uint32_t                 now = 0xFFFFFFF0u;   // wraps during the run
};

/// A FIT file: 14-byte header with ".FIT" at byte 8, then @p body bytes.
std::string fit(size_t body)
{
    std::string f(14, '\0');
    f[0] = 14;
    f[1] = 0x20;
    std::memcpy(&f[8], ".FIT", 4);
    return f + std::string(body, 'x');
}

Probe::Result runOn(TreeFileSystem& fs, FakeHost& host)
{
    // Static, as on the watch: the runner's buffers are too big for a stack.
    static Probe::Runner* runner = nullptr;
    alignas(Probe::Runner) static uint8_t storage[sizeof(Probe::Runner)];
    runner = new (storage) Probe::Runner(fs, host);
    Probe::Result r;
    r.run = 99;   // the service's field: the runner starts from a clean result
    runner->run(r);
    runner->~Runner();
    return r;
}

TreeFileSystem watchWithActivities()
{
    TreeFileSystem fs(kSandbox);
    fs.addFile("/Apps/Running/Activity/202609/activity_20260920T071500.fit", fit(100));
    fs.addFile("/Apps/Running/Activity/202609/activity_20260923T181000.fit", fit(200));
    fs.addFile("/Apps/Workout/Activity/202608/activity_20260801T060000.fit", fit(5000));
    fs.addFile("/Apps/Workout/Activity/.recording", "path");
    fs.addDir("/Apps/Settings");
    fs.addDir("/Apps/SharedData");
    return fs;
}

} // namespace

TEST(ProbeRunner, GoWhenAnotherAppsFitOpens)
{
    TreeFileSystem fs = watchWithActivities();
    FakeHost       host;
    const auto     r = runOn(fs, host);

    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_EQ(r.listParent, Check::Ok);
    EXPECT_EQ(r.listApps, Check::Ok);
    EXPECT_EQ(r.listDriveApps, Check::Ok);
    EXPECT_EQ(r.listRoot, Check::Ok);
    EXPECT_STREQ(r.base, "..");
    EXPECT_EQ(r.apps, 4u);   // HXStreakProbe, Running, Settings, Workout; SharedData excluded
    EXPECT_EQ(r.appsWithFit, 2u);
    EXPECT_EQ(r.fitFiles, 3u);
    EXPECT_EQ(r.otherFit, 0u);
    EXPECT_EQ(r.recordingMarks, 1u);
    EXPECT_STREQ(r.newest, "Running/202609/activity_20260923T181000.fit");
    EXPECT_EQ(r.fitOpen, Check::Ok);
    EXPECT_EQ(r.fitSignature, Check::Ok);
    EXPECT_EQ(r.readBytes, 5014u);   // the largest, Workout's
    EXPECT_EQ(r.readMs, 7u);         // across the clock's wrap
    EXPECT_EQ(r.sharedData, Check::Ok);
    EXPECT_EQ(r.renameRefused, Check::Ok);
    EXPECT_EQ(r.run, 0u);            // not the runner's to set
    EXPECT_TRUE(host.said("Verdict: GO"));
}

TEST(ProbeRunner, LeavesNothingBehindAndTouchesNoOtherApp)
{
    TreeFileSystem fs = watchWithActivities();
    FakeHost       host;
    runOn(fs, host);

    EXPECT_FALSE(fs.hasFile("/Apps/SharedData/hxstreak-probe.tmp"));
    EXPECT_FALSE(fs.hasFile("probe-a.tmp"));
    EXPECT_FALSE(fs.hasFile("probe-b.tmp"));
    EXPECT_EQ(fs.content("/Apps/Running/Activity/202609/activity_20260923T181000.fit"), fit(200));
    EXPECT_EQ(fs.content("/Apps/Workout/Activity/.recording"), "path");
}

TEST(ProbeRunner, BlockedWhenTheFirmwareKeepsAppsApart)
{
    TreeFileSystem fs = watchWithActivities();
    fs.blockParentAccess(true);
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.verdict, Verdict::Blocked);
    EXPECT_EQ(r.listParent, Check::Failed);
    EXPECT_EQ(r.listApps, Check::Failed);
    EXPECT_EQ(r.listDriveApps, Check::Failed);
    EXPECT_EQ(r.listRoot, Check::Failed);
    EXPECT_EQ(r.base[0], '\0');
    EXPECT_EQ(r.fitOpen, Check::NotRun);
    // The fallback routes are still measured.
    EXPECT_EQ(r.sharedData, Check::Ok);
    EXPECT_EQ(r.renameRefused, Check::Ok);
    EXPECT_TRUE(host.said("Verdict: BLOCKED"));
}

TEST(ProbeRunner, NoFilesWhenNoAppHasRecordedYet)
{
    TreeFileSystem fs(kSandbox);
    fs.addDir("/Apps/Running/Activity");
    fs.addDir("/Apps/Workout");
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.verdict, Verdict::NoFiles);
    EXPECT_EQ(r.apps, 3u);
    EXPECT_EQ(r.appsWithFit, 0u);
    EXPECT_EQ(r.readBytes, 0u);
}

TEST(ProbeRunner, NoReadWhenTheFileIsNotFit)
{
    TreeFileSystem fs(kSandbox);
    fs.addFile("/Apps/Running/Activity/202609/activity_20260920T071500.fit", "not a fit file at all");
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.verdict, Verdict::NoOpen);
    EXPECT_EQ(r.fitOpen, Check::Ok);
    EXPECT_EQ(r.fitSignature, Check::Failed);
}

TEST(ProbeRunner, OtherNamesAreCountedAndStandInWhenNoStandardFileExists)
{
    TreeFileSystem fs(kSandbox);
    fs.addFile("/Apps/Hike/Activity/202609/hike-1.FIT", fit(10));
    fs.addFile("/Apps/Hike/Activity/loose.fit", fit(10));
    fs.addFile("/Apps/Hike/Activity/202609/notes.txt", "x");
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.fitFiles, 0u);
    EXPECT_EQ(r.otherFit, 2u);
    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_STREQ(r.newest, "Hike/loose.fit");   // the first found stands in
}

TEST(ProbeRunner, FallsBackToAbsolutePathsWhenParentIsRefused)
{
    // A firmware that refuses ".." but serves absolute paths.
    class NoParentFs : public TreeFileSystem
    {
    public:
        using TreeFileSystem::TreeFileSystem;
        std::unique_ptr<SDK::Interface::IDirectory> dir(const char* path) override
        {
            return TreeFileSystem::dir(std::strcmp(path, "..") == 0 ? "/no/such/folder" : path);
        }
    };
    NoParentFs fs(kSandbox);
    fs.addFile("/Apps/Running/Activity/202609/activity_20260920T071500.fit", fit(10));
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.listParent, Check::Failed);
    EXPECT_STREQ(r.base, "/Apps");
    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_STREQ(r.newest, "Running/202609/activity_20260920T071500.fit");
}

TEST(ProbeRunner, ReadTestIsCapped)
{
    TreeFileSystem fs(kSandbox);
    fs.addFile("/Apps/Big/Activity/202609/activity_20260920T071500.fit", fit(Probe::Runner::kMaxReadBytes + 4096));
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.readBytes, Probe::Runner::kMaxReadBytes);
    EXPECT_TRUE(host.said(", capped"));
}

TEST(ProbeRunner, AppCountIsBounded)
{
    TreeFileSystem fs(kSandbox);
    for (int i = 0; i < 40; ++i) {
        fs.addDir("/Apps/App" + std::to_string(100 + i));
    }
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.apps, Probe::Runner::kMaxApps);
    EXPECT_TRUE(host.said("the rest are not scanned"));
}

TEST(ProbeRunner, ReportsRenameThatReplaces)
{
    // A file system whose rename replaces the destination (POSIX-style).
    class ReplacingFs : public TreeFileSystem
    {
    public:
        using TreeFileSystem::TreeFileSystem;
        bool rename(const char* from, const char* to) override
        {
            remove(to);
            return TreeFileSystem::rename(from, to);
        }
    };
    ReplacingFs fs(kSandbox);
    FakeHost    host;
    const auto  r = runOn(fs, host);

    EXPECT_EQ(r.renameRefused, Check::Failed);
    EXPECT_TRUE(host.said("destination now \"A\""));
}

TEST(ProbeRunner, HistoryLineIsBoundedAndComplete)
{
    Probe::Result r;
    r.run       = 3;
    r.verdict   = Verdict::Go;
    std::strcpy(r.base, "..");
    std::strcpy(r.newest, "Running/202609/activity_20260923T181000.fit");
    r.fitFiles  = 12;
    r.readBytes = 812345;
    r.readMs    = 402;
    r.sharedData = Check::Ok;
    r.renameRefused = Check::Ok;

    char       line[400];
    const auto n = Probe::Runner::historyLine(r, "2026-09-24 19:05", line, sizeof(line));
    EXPECT_EQ(n, std::strlen(line));
    EXPECT_NE(std::string(line).find("run 3 | 2026-09-24 19:05 | GO | base .."), std::string::npos);
    EXPECT_NE(std::string(line).find("read 812345 B 402 ms"), std::string::npos);
    EXPECT_NE(std::string(line).find("| by name "), std::string::npos);

    char small[16];
    const auto m = Probe::Runner::historyLine(r, "x", small, sizeof(small));
    EXPECT_EQ(m, sizeof(small) - 1);
    EXPECT_EQ(small[sizeof(small) - 1], '\0');
}

// -- [7] Exact paths ---------------------------------------------------------------

namespace
{
/// Jon's walk, as Apps/Walking/Activity/summary.json held it (28 Sep 2026).
constexpr const char* kWalkSummary =
    "{\"utc\":1790596689,\"time\":346,\"distance\":461.431,\"speed_avg\":1.33362,\"steps\":622,"
    "\"elevation\":4.26807,\"pace_avg\":0.749841,\"hr_max\":93,\"hr_avg\":81.8596,\"map\":\"6F72756F\","
    "\"lap_count\":1,\"laps\":[{\"dur\":346,\"dist\":461.431,\"steps\":622}]}";

/// Jon's watch after the walk synced: the .fit gone, the summary left.
TreeFileSystem watchAfterSync()
{
    TreeFileSystem fs(kSandbox);
    fs.addFile("/Apps/Walking/Activity/summary.json", kWalkSummary);
    fs.addDir("/Apps/Walking/Activity/202609");
    fs.addDir("/Apps/Running/Activity/202609");
    fs.addFile("/Apps/SharedData/HybridX/streak.json", "{\"v\":1}");
    return fs;
}
} // namespace

TEST(ProbeRunner, ListingRefusedButASummaryOpensByNameIsGo)
{
    TreeFileSystem fs = watchAfterSync();
    fs.blockListingOutside(true);   // what Jon's watch did
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.listParent, Check::Failed);
    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_EQ(r.summariesRead, 1u);
    EXPECT_EQ(r.summariesFound, 1u);
    EXPECT_EQ(r.appsSeen, 2u);   // Walking and Running
    EXPECT_STREQ(r.summaryApp, "Walking");
    EXPECT_STREQ(r.summaryRoute, "..");
    EXPECT_EQ(r.summaryUtc, 1790596689u);
    EXPECT_EQ(r.summarySecs, 346u);
    EXPECT_EQ(r.summaryMetres, 461u);
    EXPECT_EQ(r.sharedByName, Check::Ok);
    EXPECT_EQ(r.namesTried, 7u);
    EXPECT_TRUE(host.said("202609/: cannot list"));
    EXPECT_TRUE(host.said("Verdict: GO"));
}

TEST(ProbeRunner, SummaryAndItsFitBeforeTheSync)
{
    TreeFileSystem fs = watchAfterSync();
    fs.addFile("/Apps/Walking/Activity/202609/activity_20260928T125809.fit", fit(300));
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_EQ(r.fitByName, 1u);
    EXPECT_EQ(r.fitByNameOpen, Check::Ok);
    EXPECT_TRUE(host.said("1 .fit, first opens (.FIT ok)"));
}

TEST(ProbeRunner, BlockedWhenNothingOpensByNameEither)
{
    TreeFileSystem fs = watchAfterSync();
    fs.blockParentAccess(true);
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.verdict, Verdict::Blocked);
    EXPECT_EQ(r.appsSeen, 0u);
    EXPECT_EQ(r.summariesFound, 0u);
    EXPECT_EQ(r.sharedByName, Check::Ok);   // SharedData stays reachable
}

TEST(ProbeRunner, ASummaryWithoutAUtcIsNoRead)
{
    TreeFileSystem fs(kSandbox);
    fs.addFile("/Apps/Running/Activity/summary.json", "not json");
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.summariesFound, 1u);
    EXPECT_EQ(r.summariesRead, 0u);
    EXPECT_EQ(r.verdict, Verdict::NoOpen);
}

TEST(ProbeRunner, AppsTxtAddsNamesButNeverPaths)
{
    TreeFileSystem fs = watchAfterSync();
    fs.addFile("/Apps/Trail Run/Activity/summary.json", "{\"utc\":1790600000,\"time\":1200}");
    fs.addFile(std::string(kSandbox) + "/apps.txt",
               "Trail Run\r\n# a comment\n../Walking\nWalking\n  \nC:Evil,HXStreakProbe\n");
    FakeHost   host;
    const auto r = runOn(fs, host);

    EXPECT_EQ(r.namesTried, 8u);          // 7 built in + "Trail Run"
    EXPECT_EQ(r.summariesRead, 2u);
    EXPECT_STREQ(r.summaryApp, "Trail Run");   // the newer of the two
    EXPECT_EQ(r.summarySecs, 1200u);
    EXPECT_TRUE(host.said("apps.txt: 1 more name(s)"));
}

TEST(ProbeRunner, JsonNumber)
{
    uint32_t v = 0;
    EXPECT_TRUE(Probe::Runner::jsonNumber(kWalkSummary, "utc", v));
    EXPECT_EQ(v, 1790596689u);
    EXPECT_TRUE(Probe::Runner::jsonNumber(kWalkSummary, "distance", v));
    EXPECT_EQ(v, 461u);
    EXPECT_TRUE(Probe::Runner::jsonNumber("{ \"time\" :  12 }", "time", v));
    EXPECT_EQ(v, 12u);
    EXPECT_FALSE(Probe::Runner::jsonNumber("{\"utc\":-5}", "utc", v));
    EXPECT_FALSE(Probe::Runner::jsonNumber("{\"utc\":\"x\"}", "utc", v));
    EXPECT_FALSE(Probe::Runner::jsonNumber("{\"utc\":99999999999}", "utc", v));
    EXPECT_FALSE(Probe::Runner::jsonNumber("{}", "utc", v));
}

TEST(ProbeRunner, MonthOf)
{
    char m[7];
    Probe::Runner::monthOf(1790596689u, m);   // 2026-09-28 11:58 UTC
    EXPECT_STREQ(m, "202609");
    Probe::Runner::monthOf(0u, m);
    EXPECT_STREQ(m, "197001");
    Probe::Runner::monthOf(1772323199u, m);   // 2026-02-28 23:59:59 UTC
    EXPECT_STREQ(m, "202602");
    Probe::Runner::monthOf(1772323200u, m);   // 2026-03-01 00:00:00 UTC
    EXPECT_STREQ(m, "202603");
}

TEST(ProbeRunner, NameMatchers)
{
    EXPECT_TRUE(Probe::isMonth("202609"));
    EXPECT_FALSE(Probe::isMonth("2026-9"));
    EXPECT_FALSE(Probe::isMonth("2026091"));
    EXPECT_TRUE(Probe::isFit("a.FIT"));
    EXPECT_FALSE(Probe::isFit("fit"));
    EXPECT_TRUE(Probe::isActivityFit("activity_20260920T071500.fit"));
    EXPECT_FALSE(Probe::isActivityFit("run_20260920.fit"));
}
