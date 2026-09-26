/**
 * Host tests for Probe::Runner (Tools/Probe), the route half of Gate T0: does
 * the app find a GPX copied into its own Routes/ folder and read it into a
 * route?
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "ProbeRunner.hpp"
#include "support/FlatFileSystem.hpp"
#include "support/TestRoutes.hpp"

using Probe::Verdict;

namespace
{
class RecordingHost : public Probe::Host
{
public:
    void line(const char* text) override { lines.emplace_back(text); }
    std::vector<std::string> lines;

    bool said(const std::string& fragment) const
    {
        for (const auto& l : lines) {
            if (l.find(fragment) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
};

struct Fixture {
    FlatFileSystem               fs;
    RecordingHost                host;
    std::vector<Trail::GeoPoint> store = std::vector<Trail::GeoPoint>(2000);
    Trail::RouteBuilder          route { store.data(), 2000 };

    Probe::Result run()
    {
        Probe::Runner runner(fs, host, route);
        Probe::Result r;
        runner.run(r);
        return r;
    }
};
} // namespace

TEST(ProbeRunner, FirstRunCreatesRoutesAndSaysNoRoute)
{
    Fixture f;
    const auto r = f.run();
    EXPECT_EQ(r.verdict, Verdict::NoRoute);
    EXPECT_EQ(r.gpxCount, 0u);
    EXPECT_TRUE(f.fs.exist("Routes"));
    EXPECT_TRUE(f.host.said("copy a .gpx"));
}

TEST(ProbeRunner, ReadsTheNewestGpxIntoARoute)
{
    Fixture f;
    f.fs.addDir("Routes");
    f.fs.addFile("Routes/short_route.gpx", TestRoutes::fixture("short_route.gpx"), 100);
    f.fs.addFile("Routes/Loop.GPX", TestRoutes::loopTrack(5000), 200);   // newer, upper-case extension
    const auto r = f.run();

    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_EQ(r.gpxCount, 2u);
    EXPECT_STREQ(r.file, "Loop.GPX");
    EXPECT_STREQ(r.name, "Metadata name wins");
    EXPECT_TRUE(r.looksGpx);
    EXPECT_EQ(r.rawPoints, 5001u);
    EXPECT_EQ(r.bytesRead, r.fileBytes);
    EXPECT_GT(r.kept, 990u);
    EXPECT_NEAR(static_cast<double>(r.lengthM), 10053.0, 5.0);
    EXPECT_TRUE(r.hasEle);
    EXPECT_EQ(f.route.count(), r.kept);   // the service measures off-route against this
}

TEST(ProbeRunner, SkipsMacLeftoversFoldersAndOtherFiles)
{
    Fixture f;
    f.fs.addDir("Routes");
    f.fs.addDir("Routes/.fseventsd");
    f.fs.addFile("Routes/._short_route.gpx", std::string("\0\x05\x16\x07 AppleDouble", 16), 300);   // newest, but junk
    f.fs.addFile("Routes/.DS_Store", "junk", 300);
    f.fs.addFile("Routes/notes.txt", "hello", 300);
    f.fs.addFile("Routes/short_route.gpx", TestRoutes::fixture("short_route.gpx"), 100);
    const auto r = f.run();

    EXPECT_EQ(r.verdict, Verdict::Go);
    EXPECT_EQ(r.gpxCount, 1u);
    EXPECT_EQ(r.skipped, 4u);
    EXPECT_STREQ(r.file, "short_route.gpx");
    EXPECT_STREQ(r.name, "Fell & Back");
    EXPECT_EQ(r.rawPoints, 11u);
    EXPECT_EQ(r.lengthM, 1000u);
    EXPECT_FALSE(r.hasEle);
}

TEST(ProbeRunner, AGpxWithNoPointsIsUnreadable)
{
    Fixture f;
    f.fs.addDir("Routes");
    f.fs.addFile("Routes/empty.gpx", "<gpx><metadata><name>Nothing</name></metadata></gpx>", 1);
    const auto r = f.run();
    EXPECT_EQ(r.verdict, Verdict::Unreadable);
    EXPECT_TRUE(r.looksGpx);
    EXPECT_EQ(r.rawPoints, 0u);
}

TEST(ProbeRunner, NotReallyAGpxIsUnreadable)
{
    Fixture f;
    f.fs.addDir("Routes");
    f.fs.addFile("Routes/route.gpx", "PK\x03\x04 a zip renamed to .gpx", 1);
    const auto r = f.run();
    EXPECT_EQ(r.verdict, Verdict::Unreadable);
    EXPECT_FALSE(r.looksGpx);
    EXPECT_TRUE(f.host.said("<gpx> NOT seen"));
}

TEST(ProbeRunner, AFileNamedRoutesIsAFolderFailure)
{
    Fixture f;
    f.fs.addFile("Routes", "not a folder");
    const auto r = f.run();
    EXPECT_EQ(r.verdict, Verdict::FolderFailed);
}

TEST(ProbeRunner, IsRouteFile)
{
    EXPECT_TRUE(Probe::Runner::isRouteFile("a.gpx"));
    EXPECT_TRUE(Probe::Runner::isRouteFile("Lakes 20k.GpX"));
    EXPECT_FALSE(Probe::Runner::isRouteFile(".gpx"));
    EXPECT_FALSE(Probe::Runner::isRouteFile("._a.gpx"));
    EXPECT_FALSE(Probe::Runner::isRouteFile("a.gpx.txt"));
    EXPECT_FALSE(Probe::Runner::isRouteFile("a.fit"));
    EXPECT_FALSE(Probe::Runner::isRouteFile("gpx"));
}

TEST(ProbeRunner, AsciiFoldForTheWatchFont)
{
    char out[16];
    Probe::Runner::asciiFold("Café\tRun", out, sizeof(out));   // é is two bytes in UTF-8
    EXPECT_STREQ(out, "Caf?.Run");
    Probe::Runner::asciiFold("A very long route name indeed", out, sizeof(out));
    EXPECT_STREQ(out, "A very long rou");
}
