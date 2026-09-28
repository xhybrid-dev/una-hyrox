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

// -- Heading, turn cues, the way back, the elevation profile -------------------------

namespace
{

/// A GPX with an elevation per point: @p eleAt(i, n) metres.
template <typename F>
std::string gpxEle(const std::vector<NE>& way, F eleAt)
{
    std::string s = "<?xml version=\"1.0\"?><gpx version=\"1.1\"><trk><name>Hills</name><trkseg>";
    char        buf[128];
    const auto  pts = RunSim::route(way, 20.0);
    for (size_t i = 0; i < pts.size(); ++i) {
        std::snprintf(buf, sizeof(buf), "<trkpt lat=\"%.7f\" lon=\"%.7f\"><ele>%.1f</ele></trkpt>", pts[i].latE7 / 1e7,
                      pts[i].lonE7 / 1e7, eleAt(i, pts.size()));
        s += buf;
    }
    return s + "</trkseg></trk></gpx>";
}

std::unique_ptr<Navigator> loaded(FlatFileSystem& fs, const std::string& content)
{
    fs.addDir("Routes");
    fs.addFile("Routes/r.gpx", content, 1);
    auto nav = make(fs);
    nav->scan();
    EXPECT_TRUE(nav->load(0));
    return nav;
}

// North 300 m, then east 300 m: one right turn, 300 m in.
const std::vector<NE> kRightAngle { { 0, 0 }, { 300, 0 }, { 300, 300 } };

} // namespace

TEST(Navigator, ATurnIsCuedOnceAboutFiftyMetresBefore)
{
    FlatFileSystem fs;
    auto           nav = loaded(fs, gpx(kRightAngle, "L"));
    int            cues = 0;
    int16_t        angle = 0;
    uint16_t       distAtCue = 0;
    uint32_t       t = 0;
    for (const GeoPoint& f : RunSim::run(kRightAngle, 3.0)) {
        nav->setSpeed(true, 3.0f);
        nav->update(t += 1000, f, 4.0f, true);
        int16_t a;
        if (nav->takeTurnCue(a)) {
            ++cues;
            angle     = a;
            distAtCue = nav->status().turnDistM;
        }
    }
    EXPECT_EQ(cues, 1);
    EXPECT_NEAR(angle, 90, 15);
    EXPECT_GE(distAtCue, 40u);
    EXPECT_LE(distAtCue, 50u);
}

TEST(Navigator, NoCueWhenNoActivityIsRunningButTheTurnIsStillShown)
{
    FlatFileSystem fs;
    auto           nav = loaded(fs, gpx(kRightAngle, "L"));
    uint32_t       t = 0;
    bool           shown = false;
    for (const GeoPoint& f : RunSim::run(kRightAngle, 3.0)) {
        nav->update(t += 1000, f, 4.0f, false);
        int16_t a;
        EXPECT_FALSE(nav->takeTurnCue(a));
        shown = shown || (nav->status().turnValid && nav->status().turnDistM < 400u);
    }
    EXPECT_TRUE(shown);
}

TEST(Navigator, TheNextTurnCountsDown)
{
    FlatFileSystem fs;
    auto           nav = loaded(fs, gpx(kRightAngle, "L"));
    uint16_t       last = 65535u;
    uint32_t       t = 0;
    int            n = 0;
    for (const GeoPoint& f : RunSim::run({ { 0, 0 }, { 280, 0 } }, 3.0)) {
        nav->update(t += 1000, f, 4.0f, true);
        ASSERT_TRUE(nav->status().turnValid);
        EXPECT_LE(nav->status().turnDistM, last + 1u);   // never grows (a metre of rounding)
        last = nav->status().turnDistM;
        ++n;
    }
    EXPECT_LT(last, 30u);
    EXPECT_GT(n, 80);
}

TEST(Navigator, TheWayBackPointsAtTheNearestPartOfTheRoute)
{
    FlatFileSystem fs;
    auto           nav = loaded(fs, gpx({ { 0, 0 }, { 1000, 0 } }, "Line"));
    uint32_t       t = 0;
    for (const GeoPoint& f : RunSim::run({ { 0, 0 }, { 400, 0 } }, 3.0)) {
        nav->update(t += 1000, f, 4.0f, true);
    }
    EXPECT_FALSE(nav->status().guideValid);   // on the line
    nav->update(t += 1000, RunSim::at({ 410, 80 }), 4.0f, true);   // 80 m east of it
    const Navigator::Status& s = nav->status();
    ASSERT_TRUE(s.guideValid);
    EXPECT_FALSE(s.guideToStart);
    EXPECT_NEAR(s.guideDistM, 80.0f, 2.0f);
    EXPECT_NEAR(s.guideBearingDeg, 270.0f, 3.0f);   // due west
}

TEST(Navigator, BeforeJoiningTheRouteTheGuidePointsAtTheStart)
{
    FlatFileSystem fs;
    auto           nav = loaded(fs, gpx({ { 0, 0 }, { 1000, 0 } }, "Line"));
    nav->update(1000, RunSim::at({ -150, 0 }), 4.0f, true);   // 150 m south of the start
    const Navigator::Status& s = nav->status();
    ASSERT_TRUE(s.guideValid);
    EXPECT_TRUE(s.guideToStart);
    EXPECT_NEAR(s.guideDistM, 150.0f, 2.0f);
    EXPECT_NEAR(s.guideBearingDeg, 0.0f, 3.0f);   // north
}

TEST(Navigator, StandingStillTheCompassGivesTheHeading)
{
    FlatFileSystem fs;
    auto           nav = loaded(fs, gpx({ { 0, 0 }, { 1000, 0 } }, "Line"));
    nav->setCompass(true, 45.0f);
    for (uint32_t t = 1000; t <= 8000; t += 1000) {
        nav->setSpeed(true, 0.0f);
        nav->update(t, RunSim::at({ 10, 0 }), 4.0f, true);
    }
    const Navigator::Status& s = nav->status();
    EXPECT_TRUE(s.headingValid);
    EXPECT_EQ(s.headingSource, 2);
    EXPECT_NEAR(s.headingDeg, 45.0f, 1.0f);

    // Turning on the spot without a new fix: refreshHeading follows the compass.
    for (int i = 0; i < 20; ++i) {
        nav->setCompass(true, 200.0f);
        nav->refreshHeading();
    }
    EXPECT_NEAR(nav->status().headingDeg, 200.0f, 3.0f);
}

TEST(Navigator, RunningTheGpsHeadingWins)
{
    FlatFileSystem fs;
    auto           nav = loaded(fs, gpx({ { 0, 0 }, { 1000, 0 } }, "Line"));
    uint32_t       t = 0;
    for (const GeoPoint& f : RunSim::run({ { 0, 0 }, { 200, 0 } }, 3.0)) {
        nav->setCompass(true, 130.0f);   // a swinging wrist
        nav->setSpeed(true, 3.0f);
        nav->update(t += 1000, f, 4.0f, true);
    }
    EXPECT_EQ(nav->status().headingSource, 1);
    EXPECT_NEAR(nav->status().headingDeg, 0.0f, 3.0f);   // north
}

TEST(Navigator, WithoutTheGpsSpeedItIsEstimatedFromTheFixes)
{
    FlatFileSystem fs;
    auto           nav = loaded(fs, gpx({ { 0, 0 }, { 1000, 0 } }, "Line"));
    uint32_t       t = 0;
    for (const GeoPoint& f : RunSim::run({ { 0, 0 }, { 200, 0 } }, 3.0)) {
        nav->setCompass(true, 130.0f);
        nav->update(t += 1000, f, 4.0f, true);   // no setSpeed
    }
    EXPECT_EQ(nav->status().headingSource, 1);
}

TEST(Navigator, TheElevationProfileComesWithTheRoute)
{
    FlatFileSystem fs;
    // 1 km north, climbing 100 m over the middle 400 m.
    auto nav = loaded(fs, gpxEle({ { 0, 0 }, { 1000, 0 } }, [](size_t i, size_t n) {
        const double f = static_cast<double>(i) / static_cast<double>(n - 1);
        return f < 0.3 ? 200.0 : (f > 0.7 ? 300.0 : 200.0 + (f - 0.3) / 0.4 * 100.0);
    }));
    const auto& p = nav->profile();
    ASSERT_TRUE(p.valid());
    EXPECT_NEAR(p.minM(), 200.0f, 1.0f);
    EXPECT_NEAR(p.maxM(), 300.0f, 1.0f);
    EXPECT_NEAR(p.totalAscentM(), 100.0f, 6.0f);
    const auto c = p.nextClimb(0);
    ASSERT_TRUE(c.found);
    EXPECT_NEAR(c.startAheadM, 300.0f, 30.0f);
    EXPECT_NEAR(c.riseM, 100.0f, 6.0f);
}

TEST(Navigator, ARouteWithoutElevationHasNoProfile)
{
    FlatFileSystem fs;
    auto           nav = loaded(fs, gpx({ { 0, 0 }, { 1000, 0 } }, "Flat"));
    EXPECT_FALSE(nav->profile().valid());
}

TEST(Navigator, AThinnedLongRouteKeepsItsElevationAligned)
{
    FlatFileSystem fs;
    // 30 km: far more points than the 2,000 kept, so the builder re-thins and
    // must move the elevations with the points. Up 300 m over the first half.
    std::vector<NE> way { { 0, 0 }, { 30000, 0 } };
    std::string     s = "<?xml version=\"1.0\"?><gpx version=\"1.1\"><trk><name>Long</name><trkseg>";
    char            buf[128];
    const int       n = 6000;
    for (int i = 0; i < n; ++i) {
        const GeoPoint p = RunSim::at({ 30000.0 * i / (n - 1), 0 });
        const double   ele = i < n / 2 ? 100.0 + 300.0 * i / (n / 2) : 400.0;
        std::snprintf(buf, sizeof(buf), "<trkpt lat=\"%.7f\" lon=\"%.7f\"><ele>%.1f</ele></trkpt>", p.latE7 / 1e7,
                      p.lonE7 / 1e7, ele);
        s += buf;
    }
    s += "</trkseg></trk></gpx>";
    auto nav = loaded(fs, s);
    const auto& p = nav->profile();
    ASSERT_TRUE(p.valid());
    EXPECT_NEAR(p.elevationM(0), 100.0f, 2.0f);
    EXPECT_NEAR(p.elevationM(15000), 400.0f, 8.0f);
    EXPECT_NEAR(p.elevationM(30000), 400.0f, 2.0f);
    EXPECT_NEAR(p.totalAscentM(), 300.0f, 10.0f);
}
