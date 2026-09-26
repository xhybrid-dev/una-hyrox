/**
 ******************************************************************************
 * @file    TestRoutes.hpp
 * @brief   Synthetic GPX for the host tests: shapes with known answers.
 *
 * The same shapes as Tools/TestRoutes/make_test_gpx.py, built here so the
 * large one never needs committing. Host-only code: std::string and doubles
 * are fine.
 ******************************************************************************
 */

#ifndef TRAIL_TEST_ROUTES_HPP
#define TRAIL_TEST_ROUTES_HPP

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "GeoPoint.hpp"
#include "GpxReader.hpp"

namespace TestRoutes
{

constexpr double kLat0 = 54.45;
constexpr double kLon0 = -3.05;
constexpr double kEarthR = 6371000.0;
constexpr double kPi = 3.14159265358979323846;

/// Degrees @p north_m / @p east_m metres from (lat, lon).
inline void offset(double lat, double lon, double northM, double eastM, double& outLat, double& outLon)
{
    outLat = lat + northM / kEarthR * 180.0 / kPi;
    outLon = lon + eastM / (kEarthR * std::cos(lat * kPi / 180.0)) * 180.0 / kPi;
}

inline Trail::GeoPoint point(double lat, double lon)
{
    return Trail::GeoPoint { static_cast<int32_t>(std::lround(lat * 1e7)), static_cast<int32_t>(std::lround(lon * 1e7)) };
}

/// A GPX 1.1 track, namespaced, with times and extensions: a loop of radius
/// @p radiusM through @p n + 1 points (first == last), climbing 80 m and back.
inline std::string loopTrack(int n = 5000, double radiusM = 1600.0)
{
    std::ostringstream out;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<!-- synthetic test route: don't -> parse -> this -->\n"
           "<gpx:gpx version=\"1.1\" creator=\"t\" xmlns:gpx=\"http://www.topografix.com/GPX/1/1\">\n"
           "<gpx:metadata><gpx:name>Metadata name wins</gpx:name></gpx:metadata>\n"
           "<gpx:trk><gpx:name>Not this one</gpx:name><gpx:trkseg>\n";
    char buf[320];
    for (int i = 0; i <= n; ++i) {
        const double a = 2 * kPi * i / n;
        double       lat, lon;
        offset(kLat0, kLon0, radiusM * std::sin(a), radiusM * (1 - std::cos(a)), lat, lon);
        const double ele = 200.0 + 40.0 * (1 - std::cos(a));
        std::snprintf(buf, sizeof(buf),
                      "<gpx:trkpt lat=\"%.7f\" lon=\"%.7f\"><gpx:ele>%.1f</gpx:ele>"
                      "<gpx:time>2026-09-26T10:00:00Z</gpx:time><gpx:extensions><gpxtpx:TrackPointExtension>"
                      "<gpxtpx:hr>150</gpxtpx:hr></gpxtpx:TrackPointExtension></gpx:extensions></gpx:trkpt>\n",
                      lat, lon, ele);
        out << buf;
    }
    out << "</gpx:trkseg></gpx:trk></gpx:gpx>\n";
    return out.str();
}

inline std::string fixture(const char* name)
{
    std::ifstream      f(std::string(TRAIL_FIXTURES) + "/" + name, std::ios::binary);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

} // namespace TestRoutes

#endif // TRAIL_TEST_ROUTES_HPP
