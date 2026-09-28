/**
 ******************************************************************************
 * @file    TurnFinder.cpp
 * @brief   Turns on the route (see the header).
 ******************************************************************************
 */

#include "TurnFinder.hpp"

#include <cmath>

namespace Trail
{

GeoPoint TurnFinder::pointAt(const GeoPoint* points, uint16_t count, const float* cumulative, float alongM)
{
    if (points == nullptr || count == 0) {
        return GeoPoint {};
    }
    if (count == 1 || alongM <= 0.0f) {
        return points[0];
    }
    if (alongM >= cumulative[count - 1]) {
        return points[count - 1];
    }
    // Binary search for the segment: cumulative[lo] <= alongM < cumulative[hi].
    uint16_t lo = 0;
    uint16_t hi = static_cast<uint16_t>(count - 1);
    while (hi - lo > 1) {
        const uint16_t mid = static_cast<uint16_t>((lo + hi) / 2);
        if (cumulative[mid] <= alongM) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    const float span = cumulative[hi] - cumulative[lo];
    const float t    = span > 0.0f ? (alongM - cumulative[lo]) / span : 0.0f;
    return Geo::lerp(points[lo], points[hi], t);
}

Turn TurnFinder::next(const GeoPoint* points, uint16_t count, const float* cumulative, float fromM, float toM)
{
    Turn none;
    if (points == nullptr || cumulative == nullptr || count < 3) {
        return none;
    }
    const float total = cumulative[count - 1];
    // Scan from a little before, so a run of samples over the threshold that
    // began before @p fromM is seen whole (its peak may still be ahead).
    float first = fromM - kChordM - kGridM;
    if (first < kEndClearM) {
        first = kEndClearM;
    }
    first = std::ceil(first / kGridM) * kGridM;
    const float last = (toM + kChordM < total - kEndClearM) ? toM + kChordM : total - kEndClearM;

    bool  inRun    = false;
    float peakAt   = 0.0f;
    float peakAng  = 0.0f;   // signed
    for (float s = first; s <= last + 0.001f; s += kGridM) {
        const GeoPoint a = pointAt(points, count, cumulative, s - kChordM);
        const GeoPoint b = pointAt(points, count, cumulative, s);
        const GeoPoint c = pointAt(points, count, cumulative, s + kChordM);
        const float    ang = Geo::wrap180(Geo::bearingDeg(b, c) - Geo::bearingDeg(a, b));
        if (std::fabs(ang) >= kMinAngleDeg) {
            if (!inRun || std::fabs(ang) > std::fabs(peakAng)) {
                peakAt  = s;
                peakAng = ang;
            }
            inRun = true;
            continue;
        }
        if (inRun) {
            inRun = false;
            if (peakAt >= fromM && peakAt < toM) {
                Turn t;
                t.valid    = true;
                t.alongM   = peakAt;
                t.angleDeg = static_cast<int16_t>(std::lround(peakAng));
                return t;
            }
            peakAng = 0.0f;
        }
    }
    if (inRun && peakAt >= fromM && peakAt < toM) {
        Turn t;
        t.valid    = true;
        t.alongM   = peakAt;
        t.angleDeg = static_cast<int16_t>(std::lround(peakAng));
        return t;
    }
    return none;
}

bool TurnFinder::isSharp(int16_t angleDeg)
{
    return std::abs(angleDeg) >= static_cast<int>(kSharpDeg);
}

const char* TurnFinder::name(int16_t angleDeg)
{
    const int a = std::abs(angleDeg);
    if (a >= static_cast<int>(kUTurnDeg)) {
        return "U-turn";
    }
    if (a >= static_cast<int>(kSharpDeg)) {
        return angleDeg < 0 ? "Sharp left" : "Sharp right";
    }
    return angleDeg < 0 ? "Left" : "Right";
}

} // namespace Trail
