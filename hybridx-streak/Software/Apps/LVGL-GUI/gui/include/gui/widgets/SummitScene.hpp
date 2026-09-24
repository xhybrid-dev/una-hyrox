/**
 ******************************************************************************
 * @file    SummitScene.hpp
 * @brief   The mountain: sky, far range, the peak, its snow, the trail, the flag.
 *
 * One LVGL object that draws everything in a single LV_EVENT_DRAW_MAIN pass,
 * so the whole landscape costs one object in the 40 KB pool. It redraws only
 * when setProgress() changes something, or on every frame of a glide.
 *
 * The climber is a separate small object (Climber) so its gentle pulse
 * redraws a 24 px square, not the whole mountain.
 ******************************************************************************
 */

#ifndef STREAK_SUMMIT_SCENE_HPP
#define STREAK_SUMMIT_SCENE_HPP

#include <cstdint>

#include "lvgl.h"

#include "gui/summit/SummitGeometry.hpp"

class SummitScene
{
public:
    struct Options {
        int16_t baseY        = Summit::kHomeBaseY;
        int16_t scalePercent = 100;
        bool    drawFlag     = true;   ///< off where a separate, waving Flag is used
        bool    stars        = true;
    };

    SummitScene(lv_obj_t* parent, const Options& options);

    /// Which mountain, and how far up it: `climbed` whole steps of `steps`,
    /// plus `fraction` (0..255) of the next, for a glide in progress.
    void setProgress(uint8_t climb, uint32_t climbed, uint32_t steps, uint32_t fraction = 0);

    /// Where the climber stands now, in screen coordinates.
    Summit::Pt climberPoint() const;

    /// The apex of the main peak, in screen coordinates.
    Summit::Pt apex() const { return mMountain.main.apex; }

    lv_obj_t* obj() const { return mObj; }

private:
    static void drawCb(lv_event_t* e);
    void draw(lv_event_t* e) const;

    Options          mOptions;
    lv_obj_t*        mObj = nullptr;
    Summit::Mountain mMountain {};
    Summit::Trail    mTrail {};
    Summit::Tri      mFar[Summit::kFarRangeCount] {};
    uint8_t          mClimb    = 0;
    uint32_t         mClimbed  = 0;
    uint32_t         mSteps    = 1;
    uint32_t         mFraction = 0;
};

#endif // STREAK_SUMMIT_SCENE_HPP
