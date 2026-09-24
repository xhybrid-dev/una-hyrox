/**
 ******************************************************************************
 * @file    SummitGeometry.cpp
 * @brief   The shapes of the mountains and the trail up them (see the header).
 ******************************************************************************
 */

#include "gui/summit/SummitGeometry.hpp"

#include <cmath>

namespace Summit
{

namespace
{

/// A point given relative to the centre of the base: dx right, dy up.
struct Rel {
    int16_t dx;
    int16_t dy;
};

struct Design {
    Rel     apex, left, right;          // main peak; left/right are on the base (dy 0)
    Rel     sh[2][3];                   // shoulders: apex, left, right
    uint8_t shoulders;
    uint8_t snowPercent;
};

// Stylised, not surveyed: each silhouette is meant to be recognisable at a
// glance and distinct from the others, at 240 px across.
constexpr Design kDesigns[] = {
    // Crag -- Arthur's Seat: a long hill rising to the right, the crags a lower
    // hump on the left. No snow on an Edinburgh hill.
    { {  26, -66 }, { -76, 0 }, {  96, 0 },
      { { { -28, -38 }, { -98, 0 }, {  30, 0 } }, { { 0, 0 }, { 0, 0 }, { 0, 0 } } }, 1, 0 },
    // Pyramid -- Snowdon: a sharp peak between two ridges.
    { {   0, -84 }, { -86, 0 }, {  86, 0 },
      { { {  52, -46 }, {   0, 0 }, { 102, 0 } }, { { -54, -34 }, { -104, 0 }, { -10, 0 } } }, 2, 18 },
    // Massif -- Ben Nevis: broad and heavy, a bulky shoulder to the west.
    { {   8, -76 }, { -98, 0 }, {  96, 0 },
      { { { -40, -50 }, { -104, 0 }, {  10, 0 } }, { { 0, 0 }, { 0, 0 }, { 0, 0 } } }, 1, 24 },
    // Dome -- Mont Blanc: a wide, deep snow dome with shoulders either side.
    { {   0, -82 }, { -94, 0 }, {  94, 0 },
      { { { -50, -44 }, { -104, 0 }, {   0, 0 } }, { { 52, -48 }, { 0, 0 }, { 104, 0 } } }, 2, 44 },
    // Spire -- Everest: tall and steep, with Lhotse's shoulder behind.
    { {  -4, -92 }, { -72, 0 }, {  70, 0 },
      { { {  44, -58 }, { -10, 0 }, { 102, 0 } }, { { 0, 0 }, { 0, 0 }, { 0, 0 } } }, 1, 32 },
};

Pt place(Rel r, int16_t baseY, int16_t scalePercent)
{
    return { static_cast<int16_t>(kCentreX + r.dx * scalePercent / 100),
             static_cast<int16_t>(baseY + r.dy * scalePercent / 100) };
}

Pt lerp(Pt a, Pt b, int32_t num, int32_t den)
{
    return { static_cast<int16_t>(a.x + (b.x - a.x) * num / den),
             static_cast<int16_t>(a.y + (b.y - a.y) * num / den) };
}

/// The main peak's left or right edge x at height y.
int16_t edgeX(const Peak& p, const Pt& base, int16_t y)
{
    const int32_t h = base.y - p.apex.y;
    if (h <= 0) {
        return p.apex.x;
    }
    return static_cast<int16_t>(p.apex.x + (base.x - p.apex.x) * (y - p.apex.y) / h);
}

} // namespace

Mountain mountainFor(Streak::Shape shape, int16_t baseY, int16_t scalePercent)
{
    uint8_t i = static_cast<uint8_t>(shape);
    if (i >= sizeof(kDesigns) / sizeof(kDesigns[0])) {
        i = 0;
    }
    const Design& d = kDesigns[i];

    Mountain m {};
    m.main          = { place(d.apex, baseY, scalePercent), place(d.left, baseY, scalePercent),
                        place(d.right, baseY, scalePercent) };
    m.shoulderCount = d.shoulders;
    for (uint8_t s = 0; s < d.shoulders && s < 2; ++s) {
        m.shoulders[s] = { place(d.sh[s][0], baseY, scalePercent), place(d.sh[s][1], baseY, scalePercent),
                           place(d.sh[s][2], baseY, scalePercent) };
    }
    m.snowPercent = d.snowPercent;
    return m;
}

void farRange(Tri out[kFarRangeCount], int16_t baseY)
{
    const auto y = [baseY](int16_t up) { return static_cast<int16_t>(baseY - up); };
    out[0] = { { 38, y(32) }, { -8, baseY }, { 86, baseY } };
    out[1] = { { 98, y(40) }, { 48, baseY }, { 152, baseY } };
    out[2] = { { 164, y(34) }, { 112, baseY }, { 222, baseY } };
    out[3] = { { 214, y(26) }, { 168, baseY }, { 252, baseY } };
}

void faces(const Mountain& m, Tri& lit, Tri& shade)
{
    const Pt foot { m.main.apex.x, m.main.left.y };
    lit   = { m.main.apex, m.main.left, foot };
    shade = { m.main.apex, foot, m.main.right };
}

void snowCap(const Mountain& m, Tri& lit, Tri& shade)
{
    if (m.snowPercent == 0) {
        lit = shade = { m.main.apex, m.main.apex, m.main.apex };
        return;
    }
    const Pt a  = m.main.apex;
    const Pt l  = lerp(a, m.main.left, m.snowPercent, 100);
    const Pt r  = lerp(a, m.main.right, m.snowPercent, 100);
    // The snow line dips a little below the fall line, so the cap reads as
    // snow lying on the rock rather than a painted triangle.
    const int16_t dip = static_cast<int16_t>((l.y - a.y) / 5 + 2);
    const Pt mid { a.x, static_cast<int16_t>(l.y + dip) };
    lit   = { a, l, mid };
    shade = { a, mid, r };
}

Trail trailFor(const Mountain& m)
{
    Trail t {};
    const Peak& p = m.main;

    const int16_t yFoot = static_cast<int16_t>(p.left.y - 5);
    const int16_t yTop  = static_cast<int16_t>(p.apex.y + 9);
    const int16_t h     = static_cast<int16_t>(yFoot - yTop);
    const uint8_t legs  = h > 70 ? 4 : 3;

    // Switchbacks: start on the right of the foot, cross the face on each leg,
    // finish under the apex. The insets keep every leg well inside the flanks.
    for (uint8_t j = 0; j < legs; ++j) {
        const int16_t y  = static_cast<int16_t>(yFoot - h * j / legs);
        const int16_t xl = edgeX(p, p.left, y);
        const int16_t xr = edgeX(p, p.right, y);
        const int16_t pct = (j % 2 == 0) ? 64 : 32;
        t.points[t.count++] = { static_cast<int16_t>(xl + (xr - xl) * pct / 100), y };
    }
    t.points[t.count++] = { p.apex.x, yTop };

    t.along[0] = 0;
    for (uint8_t i = 1; i < t.count; ++i) {
        const double dx = t.points[i].x - t.points[i - 1].x;
        const double dy = t.points[i].y - t.points[i - 1].y;
        t.along[i] = t.along[i - 1] + static_cast<int32_t>(std::lround(std::sqrt(dx * dx + dy * dy) * 16.0));
    }
    return t;
}

Pt stepPosition(const Trail& t, uint32_t step, uint32_t steps, uint32_t fraction)
{
    if (t.count == 0) {
        return { kCentreX, kCentreY };
    }
    if (steps == 0 || step >= steps) {
        return t.points[t.count - 1];
    }
    if (fraction > 255u) {
        fraction = 255u;
    }

    const int64_t total  = t.along[t.count - 1];
    const int64_t target = total * (static_cast<int64_t>(step) * 256 + fraction) / (static_cast<int64_t>(steps) * 256);

    for (uint8_t i = 1; i < t.count; ++i) {
        if (target <= t.along[i]) {
            const int64_t seg = t.along[i] - t.along[i - 1];
            const int64_t in  = target - t.along[i - 1];
            if (seg <= 0) {
                return t.points[i];
            }
            const Pt a = t.points[i - 1];
            const Pt b = t.points[i];
            return { static_cast<int16_t>(a.x + (b.x - a.x) * in / seg),
                     static_cast<int16_t>(a.y + (b.y - a.y) * in / seg) };
        }
    }
    return t.points[t.count - 1];
}

} // namespace Summit
