/**
 * @file    NavigatorTest.cpp
 * @brief   Trail::Navigator: the route library, the index, the choice, and
 *          alerts only while an activity runs.
 */

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "Navigator.hpp"
#include "support/FlatFileSystem.hpp"
#include "support/RunSim.hpp"
#include "support/TestRoutes.hpp"

using RunSim::NE;
using Trail::GeoPoint;
using Trail::Navigator;
using Trail::OffCourse;

namespace
{

/// A GPX track through @p way, named @p name ("" for no name).
std::string gpx(const std::vector<NE>& way, const std::string& name)
{
    std::string s = "<?xml version=\"1.0\"?><gpx version=\"1.1\"><trk>";
    if (!name.empty()) {
        s += "<name>" + name + "</name>";
    }
    s += "<trkseg>";
    char buf[96];
    for (const GeoPoint& p : RunSim::route(way, 20.0)) {
        std::snprintf(buf, sizeof(buf), "<trkpt lat=\"%.7f\" lon=\"%.7f\"/>", p.latE7 / 1e7, p.lonE7 / 1e7);
        s += buf;
    }
    return s + "</trkseg></trk></gpx>";
}

const std::vector<NE> kLine { { 0, 0 }, { 1000, 0 } };
const std::vector<NE> kLoop { { 0, 0 }, { 500, 0 }, { 500, 500 }, { 0, 500 }, { 0, 0 } };

/// Navigator is 26 KB: on the heap here, in static storage on the watch.
std::unique_ptr<Navigator> make(FlatFileSystem& fs)
{
    return std::make_unique<Navigator>(fs);
}

} // namespace

TEST(Navigator, ScanListsRoutesByNameAndSkipsTheRest)
{
    FlatFileSystem fs;
    fs.addDir("Routes");
    fs.addFile("Routes/b.gpx", gpx(kLine, "Zig Zag"), 10);
    fs.addFile("Routes/a.GPX", gpx(kLoop, "Abbey Loop"), 20);
    fs.addFile("Routes/noname.gpx", gpx(kLine, ""), 30);
    fs.addFile("Routes/._a.GPX", "mac junk", 40);
    fs.addFile("Routes/notes.txt", "hello", 50);
    auto nav = make(fs);

    ASSERT_EQ(nav->scan(), 3u);
    EXPECT_STREQ(nav->routes()[0].name, "Abbey Loop");
    EXPECT_STREQ(nav->routes()[1].name, "noname");   // the file name, without ".gpx"
    EXPECT_STREQ(nav->routes()[2].name, "Zig Zag");
    EXPECT_NEAR(nav->routes()[0].lengthM, 2000u, 5u);
    EXPECT_NEAR(nav->routes()[2].lengthM, 1000u, 5u);
    EXPECT_GT(nav->routes()[0].points, 1u);
    EXPECT_EQ(nav->parses(), 3u);
    EXPECT_TRUE(fs.hasFile("routes.idx"));
}

TEST(Navigator, TheIndexMeansASecondScanParsesNothing)
{
    FlatFileSystem fs;
    fs.addDir("Routes");
    fs.addFile("Routes/a.gpx", gpx(kLoop, "Abbey Loop"), 20);
    fs.addFile("Routes/b.gpx", gpx(kLine, "Beacon"), 10);
    {
        auto nav = make(fs);
        nav->scan();
    }
    auto nav = make(fs);   // a new app start
    ASSERT_EQ(nav->scan(), 2u);
    EXPECT_EQ(nav->parses(), 0u);
    EXPECT_STREQ(nav->routes()[0].name, "Abbey Loop");
    EXPECT_NEAR(nav->routes()[0].lengthM, 2000u, 5u);

    // A changed file (new date) is parsed again; the other is not.
    fs.addFile("Routes/b.gpx", gpx(kLoop, "Beacon Loop"), 99);
    nav->scan();
    EXPECT_EQ(nav->parses(), 1u);
    EXPECT_STREQ(nav->routes()[1].name, "Beacon Loop");
}

TEST(Navigator, AGarbledIndexIsIgnored)
{
    FlatFileSystem fs;
    fs.addDir("Routes");
    fs.addFile("Routes/a.gpx", gpx(kLoop, "Abbey Loop"), 20);
    fs.addFile("routes.idx", "not an index\nat all\n");
    auto nav = make(fs);
    ASSERT_EQ(nav->scan(), 1u);
    EXPECT_EQ(nav->parses(), 1u);
    EXPECT_STREQ(nav->routes()[0].name, "Abbey Loop");
}

TEST(Navigator, LoadingRemembersTheChoice)
{
    FlatFileSystem fs;
    fs.addDir("Routes");
    fs.addFile("Routes/a.gpx", gpx(kLoop, "Abbey Loop"), 20);
    fs.addFile("Routes/b.gpx", gpx(kLine, "Beacon"), 10);
    {
        auto nav = make(fs);
        nav->scan();
        ASSERT_TRUE(nav->load(1));
        EXPECT_TRUE(nav->loaded());
        EXPECT_STREQ(nav->current().name, "Beacon");
        EXPECT_GT(nav->pointCount(), 1u);
        EXPECT_EQ(fs.content("route.sel"), "b.gpx");
    }
    auto nav = make(fs);
    nav->scan();
    ASSERT_TRUE(nav->restoreSelection());
    EXPECT_EQ(nav->selected(), 1);
    EXPECT_STREQ(nav->current().name, "Beacon");

    nav->clear();
    EXPECT_FALSE(nav->loaded());
    EXPECT_EQ(nav->selected(), -1);
    EXPECT_FALSE(fs.hasFile("route.sel"));
}

TEST(Navigator, ADeletedRouteIsForgotten)
{
    FlatFileSystem fs;
    fs.addDir("Routes");
    fs.addFile("Routes/a.gpx", gpx(kLoop, "Abbey Loop"), 20);
    auto nav = make(fs);
    nav->scan();
    ASSERT_TRUE(nav->load(0));
    fs.remove("Routes/a.gpx");
    EXPECT_EQ(nav->scan(), 0u);
    EXPECT_FALSE(nav->loaded());
    EXPECT_FALSE(nav->restoreSelection());
}

TEST(Navigator, AnUnreadableRouteDoesNotLoad)
{
    FlatFileSystem fs;
    fs.addDir("Routes");
    fs.addFile("Routes/bad.gpx", "PK\x03\x04 a zip, renamed", 20);
    auto nav = make(fs);
    ASSERT_EQ(nav->scan(), 1u);
    EXPECT_EQ(nav->routes()[0].points, 0u);
    EXPECT_FALSE(nav->load(0));
    EXPECT_FALSE(nav->loaded());
}

TEST(Navigator, ScanCreatesTheRoutesFolder)
{
    FlatFileSystem fs;
    auto           nav = make(fs);
    EXPECT_EQ(nav->scan(), 0u);
    EXPECT_TRUE(fs.exist("Routes"));
}

TEST(Navigator, AlertsOnlyWhileAnActivityRuns)
{
    FlatFileSystem fs;
    fs.addDir("Routes");
    fs.addFile("Routes/b.gpx", gpx(kLine, "Beacon"), 10);
    auto nav = make(fs);
    nav->scan();
    ASSERT_TRUE(nav->load(0));

    // On the start screen: on the route, then wandering 200 m off. No alerts.
    uint32_t ms = 0;
    nav->update(ms += 1000, RunSim::at({ 0, 0 }), 3.0f, false);
    EXPECT_TRUE(nav->status().pos.everLocked);
    EXPECT_NEAR(nav->status().toStartM, 0.0f, 1.0f);
    for (int i = 0; i < 30; ++i) {
        EXPECT_EQ(nav->update(ms += 1000, RunSim::at({ 100, 200 }), 3.0f, false), OffCourse::Event::None);
    }
    EXPECT_EQ(nav->status().alert, OffCourse::State::NotStarted);

    // The run starts; the runner goes the wrong way: one alert after 5 s.
    nav->resetProgress();
    nav->update(ms += 1000, RunSim::at({ 0, 0 }), 3.0f, true);
    std::vector<OffCourse::Event> events;
    for (int i = 0; i < 10; ++i) {
        const auto e = nav->update(ms += 1000, RunSim::at({ 100, 200 }), 3.0f, true);
        if (e != OffCourse::Event::None) {
            events.push_back(e);
        }
    }
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0], OffCourse::Event::WentOff);
    EXPECT_EQ(nav->status().alert, OffCourse::State::Off);
    EXPECT_GE(nav->status().offForS, 4u);
}

TEST(Navigator, WithNoRouteItStillTracksHeading)
{
    FlatFileSystem fs;
    auto           nav = make(fs);
    nav->update(1000, RunSim::at({ 0, 0 }), 3.0f, true);
    nav->update(2000, RunSim::at({ 20, 0 }), 3.0f, true);
    EXPECT_TRUE(nav->status().hasFix);
    EXPECT_TRUE(nav->status().headingValid);
    EXPECT_NEAR(nav->status().headingDeg, 0.0f, 1.0f);
    EXPECT_FALSE(nav->status().routeLoaded);
}
