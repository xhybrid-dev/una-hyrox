/**
 ******************************************************************************
 * @file    ProbeScreen.hpp
 * @brief   The Q0 feasibility screen: a scene with two animated sprites, the
 *          four-button bezel, the timing beat, and live numbers for Gate Q0.
 *
 * Buttons: L1 plays the selected feedback pattern, L2 selects the next one,
 * R1 presses the beat, R2 leaves.
 *
 * What it measures, on the screen itself so Jon can read it on the watch:
 *   - "r a/b ms": frame render time, last and worst in the last second;
 *   - "f": frames rendered in that second (10 is the kernel's tick rate);
 *   - "p": LVGL pool use, per cent;
 *   - "rt": round trip of a beat press, button to the service's answer (ms);
 *   - the last three button codes in the order they arrived: press (q w e r),
 *     click (1 2 3 4) and release (a s d f), and so which comes first.
 ******************************************************************************
 */

#ifndef TRAILQUEST_PROBE_SCREEN_HPP
#define TRAILQUEST_PROBE_SCREEN_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "Beat.hpp"
#include "gui/Sprite.hpp"
#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class ProbeScreen : public ModelListener
{
public:
    explicit ProbeScreen(Model& model);
    ~ProbeScreen();

    ProbeScreen(const ProbeScreen&)            = delete;
    ProbeScreen& operator=(const ProbeScreen&) = delete;

    /// Build the widgets, load the screen and bind to the model.
    void show();

    void onBeatResult(const CustomMessage::BeatResult& r) override;

private:
    static void keyEventCb(lv_event_t* e);
    static void tickCb(lv_timer_t* t);
    static void renderCb(lv_event_t* e);

    void onKey(uint8_t code);
    void onTick();
    void updateStats();
    void updateKeys();

    Model&    mModel;
    lv_obj_t* mRoot = nullptr;

    XTrailQuest::BeatConfig mBeat;
    uint32_t mBeatStart = 0;
    uint32_t mSeq       = 0;
    uint32_t mPressTick = 0;     ///< When the last press was sent.

    Sprite mRunner;
    Sprite mStitch;
    Sprite mBonk;

    lv_obj_t* mNeedle   = nullptr;
    lv_obj_t* mResult   = nullptr;
    lv_obj_t* mStats    = nullptr;
    lv_obj_t* mKeys     = nullptr;
    lv_obj_t* mFxName   = nullptr;
    lv_timer_t* mTimer  = nullptr;
    std::unique_ptr<SDK::LVGL::Buttons> mButtons;

    uint8_t mFx = 0;
    uint8_t mFrame = 0;
    uint32_t mLastFlip = 0;

    // Render timing, measured between LVGL's render start and ready events.
    uint32_t mRenderStart = 0;
    uint32_t mRenderLast  = 0;
    uint32_t mRenderMax   = 0;
    uint32_t mFrames      = 0;
    uint32_t mStatsAt     = 0;
    uint32_t mRoundTrip   = 0;

    char mKeyText[16] = {0};
};

#endif // TRAILQUEST_PROBE_SCREEN_HPP
