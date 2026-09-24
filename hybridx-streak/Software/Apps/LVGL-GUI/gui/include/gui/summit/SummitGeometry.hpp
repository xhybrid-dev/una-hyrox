/**
 ******************************************************************************
 * @file    SummitGeometry.hpp
 * @brief   The shapes of the mountains and the trail up them. Pure C++.
 *
 * Everything the scene draws is described here as points and triangles in
 * scene coordinates (240 x 240, the screen's own). No LVGL: the host tests
 * compile this file directly and check that every step of every trail sits on
 * the mountain, inside the round display's safe circle, and climbs.
 *
 * A mountain is a main peak (the one the trail climbs, drawn in a sunlit and
 * a shaded half), up to two shoulders behind it, and a snow cap. The far range
 * behind everything is shared by all mountains.
 ******************************************************************************
 */

#ifndef STREAK_SUMMIT_GEOMETRY_HPP
#define STREAK_SUMMIT_GEOMETRY_HPP

#include <cstdint>

#include "Summits.hpp"

namespace Summit
{

struct Pt {
    int16_t x;
    int16_t y;
};

struct Tri {
    Pt a;
    Pt b;
    Pt c;
};

/// The main peak: apex and the two base corners.
struct Peak {
    Pt apex;
    Pt left;
    Pt right;
};

struct Mountain {
    Peak    main;
    Tri     shoulders[2];
    uint8_t shoulderCount;
    /// Snow reaches this fraction of the way down the main peak, in percent;
    /// 0 for no snow.
    uint8_t snowPercent;
};

/// Where the scene's content may go: clear of the bezel's button-hint arcs
/// (radius 107-117 from the centre, Buttons.cpp) with a margin.
constexpr int16_t kCentreX    = 120;
constexpr int16_t kCentreY    = 120;
constexpr int16_t kSafeRadius = 100;

/// Base line of the home screen's scene.
constexpr int16_t kHomeBaseY = 118;

/// The mountain for a climb's shape, drawn with its base on `baseY` and
/// scaled by `scalePercent` about the base's centre (100 = home screen size).
Mountain mountainFor(Streak::Shape shape, int16_t baseY = kHomeBaseY, int16_t scalePercent = 100);

/// The far range, shared by every mountain.
constexpr uint8_t kFarRangeCount = 4;
void farRange(Tri out[kFarRangeCount], int16_t baseY = kHomeBaseY);

/// The snow cap's lit and shaded halves (empty when snowPercent is 0).
void snowCap(const Mountain& m, Tri& lit, Tri& shade);

/// The main peak's sunlit (left) and shaded (right) halves.
void faces(const Mountain& m, Tri& lit, Tri& shade);

/// A switchback trail up the main peak, from the foot to just under the apex.
struct Trail {
    static constexpr uint8_t kMaxPoints = 8;
    Pt      points[kMaxPoints];
    uint8_t count;
    /// Cumulative length at each point, in 1/16 px (so the arithmetic stays integer).
    int32_t along[kMaxPoints];
};

Trail trailFor(const Mountain& m);

/**
 * Position `step` of `steps` along the trail: 0 is the foot, `steps` the top.
 * `fraction` (0..255) moves a further part of one step along, for the climber
 * gliding between steps.
 */
Pt stepPosition(const Trail& t, uint32_t step, uint32_t steps, uint32_t fraction = 0);


} // namespace Summit

#endif // STREAK_SUMMIT_GEOMETRY_HPP
