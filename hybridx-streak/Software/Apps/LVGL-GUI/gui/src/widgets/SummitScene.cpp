/**
 ******************************************************************************
 * @file    SummitScene.cpp
 * @brief   The mountain (see the header).
 ******************************************************************************
 */

#include "gui/widgets/SummitScene.hpp"

#include "SDK/Port/LVGL/LvglPort.hpp"

#include "Summits.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/widgets/Paint.hpp"

using Summit::Pt;

namespace
{

/// A few fixed stars, placed clear of every peak's flag.
constexpr Pt kStars[] = {
    { 62, 34 }, { 88, 20 }, { 158, 24 }, { 186, 44 }, { 44, 58 }, { 204, 70 }, { 136, 12 },
};

/// Steps are drawn as dots only where they are far enough apart to read as
/// steps: the first two climbs (4 and 8 weeks). On the big mountains the lime
/// trail itself shows the progress (DESIGN.md "The scene").
constexpr uint32_t kMaxDottedSteps = 8;

} // namespace

SummitScene::SummitScene(lv_obj_t* parent, const Options& options)
    : mOptions(options)
{
    mObj = Paint::surface(parent, 0, 0, SDK::LVGL::kDisplayWidth, mOptions.baseY + 2, &SummitScene::drawCb, this);
    Summit::farRange(mFar, mOptions.baseY);
    setProgress(0, 0, Streak::kClimbs[0].steps);
}

void SummitScene::setProgress(uint8_t climb, uint32_t climbed, uint32_t steps, uint32_t fraction)
{
    if (climb >= Streak::kClimbCount) {
        climb = Streak::kClimbCount - 1;
    }
    if (steps == 0) {
        steps = 1;
    }
    if (climbed > steps) {
        climbed = steps;
    }
    const bool newMountain = climb != mClimb || mTrail.count == 0;
    mClimb    = climb;
    mClimbed  = climbed;
    mSteps    = steps;
    mFraction = fraction;
    if (newMountain) {
        mMountain = Summit::mountainFor(Streak::kClimbs[climb].shape, mOptions.baseY, mOptions.scalePercent);
        mTrail    = Summit::trailFor(mMountain);
    }
    lv_obj_invalidate(mObj);
}

Pt SummitScene::climberPoint() const
{
    return Summit::stepPosition(mTrail, mClimbed, mSteps, mFraction);
}

void SummitScene::drawCb(lv_event_t* e)
{
    static_cast<const SummitScene*>(lv_event_get_user_data(e))->draw(e);
}

void SummitScene::draw(lv_event_t* e) const
{
    const Paint::Canvas c = Paint::Canvas::of(e);

    if (mOptions.stars) {
        for (uint8_t i = 0; i < sizeof(kStars) / sizeof(kStars[0]); ++i) {
            Paint::rect(c, kStars[i].x, kStars[i].y, 2, 2, (i % 3 == 0) ? Palette::kStarLit : Palette::kStar);
        }
    }

    // Far range, then the shoulders, then the peak lit on its left (the sun is
    // in the west) and shaded on its right, then its snow.
    for (const auto& t : mFar) {
        Paint::tri(c, t, Palette::kFarRange);
    }
    for (uint8_t s = 0; s < mMountain.shoulderCount; ++s) {
        Paint::tri(c, mMountain.shoulders[s], Palette::kRockShade);
    }
    Summit::Tri lit, shade;
    Summit::faces(mMountain, lit, shade);
    Paint::tri(c, lit, Palette::kRockLit);
    Paint::tri(c, shade, Palette::kRockShade);
    if (mMountain.snowPercent > 0) {
        Summit::snowCap(mMountain, lit, shade);
        Paint::tri(c, lit, Palette::kSnowLit);
        Paint::tri(c, shade, Palette::kSnowShade);
    }

    // The trail: lime behind the climber, grey ahead.
    const Pt here = climberPoint();
    const int64_t hereAlong = static_cast<int64_t>(mTrail.along[mTrail.count - 1]) *
                              (static_cast<int64_t>(mClimbed) * 256 + mFraction) /
                              (static_cast<int64_t>(mSteps) * 256);
    for (uint8_t i = 1; i < mTrail.count; ++i) {
        const Pt a = mTrail.points[i - 1];
        const Pt b = mTrail.points[i];
        if (mTrail.along[i] <= hereAlong) {
            Paint::line(c, a, b, 3, Palette::kWin);
        } else if (mTrail.along[i - 1] >= hereAlong) {
            Paint::line(c, a, b, 1, Palette::kTrail);
        } else {
            Paint::line(c, here, b, 1, Palette::kTrail);
            Paint::line(c, a, here, 3, Palette::kWin);
        }
    }
    if (mSteps <= kMaxDottedSteps) {
        for (uint32_t s = 1; s < mSteps; ++s) {
            const Pt p = Summit::stepPosition(mTrail, s, mSteps);
            if (s <= mClimbed) {
                Paint::disc(c, p, 3, Palette::kWin);
            } else {
                Paint::disc(c, p, 2, Palette::kTrail);
            }
        }
    }

    if (mOptions.drawFlag) {
        const Pt top = mMountain.main.apex;
        const Pt poleTop { top.x, static_cast<int16_t>(top.y - 15) };
        Paint::line(c, top, poleTop, 2, Palette::kText);
        Paint::tri(c, { static_cast<int16_t>(top.x + 1), poleTop.y },
                   { static_cast<int16_t>(top.x + 12), static_cast<int16_t>(poleTop.y + 4) },
                   { static_cast<int16_t>(top.x + 1), static_cast<int16_t>(poleTop.y + 8) }, Palette::kWin);
    }
}
