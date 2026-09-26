/**
 * @file    GpxReaderTest.cpp
 * @brief   Trail::GpxReader: points out of GPX, however the bytes arrive.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "GpxReader.hpp"
#include "support/TestRoutes.hpp"

using Trail::GeoPoint;
using Trail::GpxReader;
using Trail::PointKind;

namespace
{

struct Got {
    GeoPoint  p;
    bool      hasEle;
    int32_t   eleCm;
    PointKind kind;
};

class Collect : public Trail::GpxSink
{
public:
    void point(const GeoPoint& p, bool hasEle, int32_t eleCm, PointKind kind) override
    {
        points.push_back(Got { p, hasEle, eleCm, kind });
    }
    std::vector<Got> points;
};

/// Feed @p gpx in chunks of @p chunk bytes (0 = all at once).
GpxReader::Stats read(const std::string& gpx, Collect& sink, size_t chunk = 0)
{
    GpxReader r(sink);
    if (chunk == 0) {
        r.feed(gpx.data(), gpx.size());
    } else {
        for (size_t i = 0; i < gpx.size(); i += chunk) {
            r.feed(gpx.data() + i, std::min(chunk, gpx.size() - i));
        }
    }
    return r.stats();
}

} // namespace

TEST(GpxReaderParseFixed, Coordinates)
{
    int64_t v = 0;
    ASSERT_TRUE(GpxReader::parseFixed("54.4500000", 10, 7, v));
    EXPECT_EQ(v, 544500000);
    ASSERT_TRUE(GpxReader::parseFixed(" -3.05 ", 7, 7, v));
    EXPECT_EQ(v, -30500000);
    ASSERT_TRUE(GpxReader::parseFixed("+1", 2, 7, v));
    EXPECT_EQ(v, 10000000);
    ASSERT_TRUE(GpxReader::parseFixed("-0.123456789", 12, 7, v));   // rounds on the 8th digit
    EXPECT_EQ(v, -1234568);
    ASSERT_TRUE(GpxReader::parseFixed(".5", 2, 2, v));
    EXPECT_EQ(v, 50);
    ASSERT_TRUE(GpxReader::parseFixed("123.", 4, 2, v));
    EXPECT_EQ(v, 12300);
}

TEST(GpxReaderParseFixed, RefusesWhatIsNotAPlainDecimal)
{
    int64_t v = 0;
    for (const char* bad : { "", " ", "-", ".", "1e5", "1,5", "1.2.3", "abc", "12a", "1 2", "99999999999" }) {
        EXPECT_FALSE(GpxReader::parseFixed(bad, std::strlen(bad), 7, v)) << bad;
    }
}

TEST(GpxReader, ReadsANamespacedTrackWithElevation)
{
    Collect    sink;
    const auto s = read(TestRoutes::loopTrack(100), sink);
    EXPECT_TRUE(s.sawGpx);
    EXPECT_EQ(s.trackPoints, 101u);
    EXPECT_EQ(s.routePoints, 0u);
    EXPECT_EQ(s.badPoints, 0u);
    EXPECT_STREQ(s.name, "Metadata name wins");
    ASSERT_EQ(sink.points.size(), 101u);
    EXPECT_EQ(sink.points[0].p.latE7, 544500000);
    EXPECT_EQ(sink.points[0].p.lonE7, -30500000);
    EXPECT_TRUE(sink.points[0].hasEle);
    EXPECT_EQ(sink.points[0].eleCm, 20000);
    EXPECT_EQ(sink.points[50].eleCm, 28000);   // halfway round: the top of the climb
    EXPECT_EQ(sink.points[0].kind, PointKind::Track);
}

TEST(GpxReader, TheSameWhateverTheChunkSize)
{
    const std::string gpx = TestRoutes::loopTrack(200);
    Collect           whole;
    read(gpx, whole);
    for (size_t chunk : { 1u, 2u, 3u, 7u, 64u, 511u, 512u }) {
        Collect    sink;
        const auto s = read(gpx, sink, chunk);
        ASSERT_EQ(sink.points.size(), whole.points.size()) << "chunk " << chunk;
        EXPECT_STREQ(s.name, "Metadata name wins");
        for (size_t i = 0; i < sink.points.size(); ++i) {
            ASSERT_EQ(sink.points[i].p.latE7, whole.points[i].p.latE7) << chunk << " " << i;
            ASSERT_EQ(sink.points[i].p.lonE7, whole.points[i].p.lonE7) << chunk << " " << i;
            ASSERT_EQ(sink.points[i].eleCm, whole.points[i].eleCm) << chunk << " " << i;
        }
    }
}

TEST(GpxReader, ReadsARouteWithSingleQuotesCrLfAndEntities)
{
    Collect    sink;
    const auto s = read(TestRoutes::fixture("short_route.gpx"), sink, 5);
    EXPECT_TRUE(s.sawGpx);
    EXPECT_EQ(s.routePoints, 11u);
    EXPECT_EQ(s.trackPoints, 0u);
    EXPECT_EQ(s.waypoints, 1u);
    EXPECT_STREQ(s.name, "Fell & Back");   // not the waypoint's "Summit cairn"
    ASSERT_EQ(sink.points.size(), 11u);
    EXPECT_EQ(sink.points[0].kind, PointKind::Route);
    EXPECT_FALSE(sink.points[0].hasEle);
}

TEST(GpxReader, SelfClosingPointsAndAttributeOrder)
{
    Collect    sink;
    const auto s = read("<gpx><trk><trkseg>"
                        "<trkpt lon=\"-3.1\" lat=\"54.2\"/>"
                        "<trkpt  lat = '54.3'  lon = '-3.2' />"
                        "</trkseg></trk></gpx>",
                        sink);
    EXPECT_EQ(s.trackPoints, 2u);
    ASSERT_EQ(sink.points.size(), 2u);
    EXPECT_EQ(sink.points[0].p.latE7, 542000000);
    EXPECT_EQ(sink.points[0].p.lonE7, -31000000);
    EXPECT_EQ(sink.points[1].p.latE7, 543000000);
}

TEST(GpxReader, DropsBadPointsNeverInventsThem)
{
    Collect    sink;
    const auto s = read("<gpx><trk><trkseg>"
                        "<trkpt lat=\"54.2\"></trkpt>"                  // no lon
                        "<trkpt lat=\"91\" lon=\"0\"></trkpt>"          // out of range
                        "<trkpt lat=\"5e1\" lon=\"0\"></trkpt>"         // exponent
                        "<trkpt lat=54.2 lon=-3.1></trkpt>"              // unquoted
                        "<trkpt lat=\"54.2\" lon=\"-3.1\"><ele>high</ele></trkpt>"   // good point, bad ele
                        "</trkseg></trk></gpx>",
                        sink);
    EXPECT_EQ(s.badPoints, 4u);
    EXPECT_EQ(s.trackPoints, 1u);
    ASSERT_EQ(sink.points.size(), 1u);
    EXPECT_FALSE(sink.points[0].hasEle);
}

TEST(GpxReader, SkipsCommentsCdataAndDeclarations)
{
    Collect    sink;
    const auto s = read("<?xml version='1.0'?><!DOCTYPE gpx>"
                        "<gpx><!-- <trkpt lat=\"1\" lon=\"1\"></trkpt> it's > ignored -->"
                        "<trk><name><![CDATA[Ridge > Valley]]></name><trkseg>"
                        "<trkpt lat=\"54.2\" lon=\"-3.1\"><desc><![CDATA[<trkpt lat=\"2\">]]></desc></trkpt>"
                        "</trkseg></trk></gpx>",
                        sink, 3);
    EXPECT_EQ(s.trackPoints, 1u);
    EXPECT_EQ(s.badPoints, 0u);
    EXPECT_STREQ(s.name, "Ridge > Valley");
}

TEST(GpxReader, NotGpxGivesNothing)
{
    Collect    sink;
    const auto s = read("PK\x03\x04 this is a zip, or a FIT file, or anything else <<>>", sink);
    EXPECT_FALSE(s.sawGpx);
    EXPECT_EQ(sink.points.size(), 0u);
}

TEST(GpxReader, ALongTagIsCountedAndItsPointStillRead)
{
    // lat/lon come first; a very long tail of other attributes is cut off.
    std::string tag = "<trkpt lat=\"54.2\" lon=\"-3.1\" note=\"" + std::string(400, 'x') + "\">";
    Collect     sink;
    const auto  s = read("<gpx><trk><trkseg>" + tag + "</trkpt></trkseg></trk></gpx>", sink);
    EXPECT_EQ(s.longTags, 1u);
    EXPECT_EQ(s.trackPoints, 1u);
}

TEST(GpxReader, NameIsTrimmedAndCollapsed)
{
    Collect    sink;
    const auto s = read("<gpx><rte><name>\n   Lakes   20k\t&lt;loop&gt;  </name></rte></gpx>", sink);
    EXPECT_STREQ(s.name, "Lakes 20k <loop>");
}

TEST(GpxReader, ResetStartsAgain)
{
    Collect   sink;
    GpxReader r(sink);
    const std::string half = "<gpx><trk><name>First</name><trkseg><trkpt lat=\"1\" lo";
    r.feed(half.data(), half.size());
    r.reset();
    const std::string next = TestRoutes::fixture("short_route.gpx");
    r.feed(next.data(), next.size());
    EXPECT_STREQ(r.stats().name, "Fell & Back");
    EXPECT_EQ(r.stats().routePoints, 11u);
    EXPECT_EQ(r.stats().trackPoints, 0u);
}
