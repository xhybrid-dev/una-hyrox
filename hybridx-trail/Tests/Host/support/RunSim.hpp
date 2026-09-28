/**
 ******************************************************************************
 * @file    RunSim.hpp
 * @brief   Synthetic routes and runs for the tracking tests.
 *
 * Shapes are drawn in metres (north, east) from a fixed origin, then turned
 * into GeoPoints. A "run" walks the same shape (or a different one: a wrong
 * turn, a shortcut) and yields one fix per step, like 1 Hz GPS, with optional
 * noise from a fixed-seed generator so every test run is identical.
 * Host-only: std::vector and doubles are fine here.
 ******************************************************************************
 */

#ifndef TRAIL_RUN_SIM_HPP
#define TRAIL_RUN_SIM_HPP

#include <cmath>
#include <cstdint>
#include <vector>

#include "GeoPoint.hpp"
#include "support/TestRoutes.hpp"

namespace RunSim
{

struct NE {
    double n;   ///< metres north of the origin
    double e;   ///< metres east
};

inline Trail::GeoPoint at(const NE& p)
{
    double lat, lon;
    TestRoutes::offset(TestRoutes::kLat0, TestRoutes::kLon0, p.n, p.e, lat, lon);
    return TestRoutes::point(lat, lon);
}

/// Points every @p stepM along the polyline through @p way, and its last
/// point. With @p keepCorners every corner is a point too (a route, as a GPX
/// has them); without, the steps run straight through them (a run: one point
/// per second at @p stepM m/s).
inline std::vector<NE> densify(const std::vector<NE>& way, double stepM, bool keepCorners = false)
{
    std::vector<NE> out;
    if (way.empty()) {
        return out;
    }
    out.push_back(way[0]);
    double carry = 0.0;   // distance already walked into the current step
    for (size_t i = 1; i < way.size(); ++i) {
        const double dn  = way[i].n - way[i - 1].n;
        const double de  = way[i].e - way[i - 1].e;
        const double len = std::sqrt(dn * dn + de * de);
        double       s   = stepM - carry;
        while (s < len) {
            out.push_back(NE { way[i - 1].n + dn * s / len, way[i - 1].e + de * s / len });
            s += stepM;
        }
        carry = len - (s - stepM);
        if (keepCorners && i + 1 < way.size()) {
            out.push_back(way[i]);
            carry = 0.0;
        }
    }
    const NE& last = way.back();
    if (out.back().n != last.n || out.back().e != last.e) {
        out.push_back(last);
    }
    return out;
}

inline std::vector<Trail::GeoPoint> route(const std::vector<NE>& way, double spacingM = 25.0)
{
    std::vector<Trail::GeoPoint> pts;
    for (const NE& p : densify(way, spacingM, true)) {
        pts.push_back(at(p));
    }
    return pts;
}

/// A tiny deterministic generator: noise in [-amp, +amp].
class Noise
{
public:
    explicit Noise(uint32_t seed = 12345u) : mState(seed) {}
    double next(double amp)
    {
        mState = mState * 1664525u + 1013904223u;
        return amp * ((static_cast<double>(mState >> 8) / 16777216.0) * 2.0 - 1.0);
    }

private:
    uint32_t mState;
};

/// Fixes along @p way at @p speedMps, one a second, each moved by up to
/// @p noiseM in north and east.
inline std::vector<Trail::GeoPoint> run(const std::vector<NE>& way, double speedMps = 3.0, double noiseM = 0.0,
                                        uint32_t seed = 12345u)
{
    Noise                        noise(seed);
    std::vector<Trail::GeoPoint> fixes;
    for (NE p : densify(way, speedMps)) {
        p.n += noise.next(noiseM);
        p.e += noise.next(noiseM);
        fixes.push_back(at(p));
    }
    return fixes;
}

/// Length of the polyline through @p way, metres.
inline double length(const std::vector<NE>& way)
{
    double total = 0.0;
    for (size_t i = 1; i < way.size(); ++i) {
        total += std::hypot(way[i].n - way[i - 1].n, way[i].e - way[i - 1].e);
    }
    return total;
}

} // namespace RunSim

#endif // TRAIL_RUN_SIM_HPP
