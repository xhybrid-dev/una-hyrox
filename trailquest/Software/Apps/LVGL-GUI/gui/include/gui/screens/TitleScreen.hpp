/**
 ******************************************************************************
 * @file    TitleScreen.hpp
 * @brief   The title: play on (or start), a new run over a save, the beat on
 *          or off.
 *
 *   R1  Play: carry on where the save left off, or start a run if there is none
 *   L1  held: a new run, over the save (only shown when there is one; a quick
 *       press says to hold it)
 *   L2  the timing beat on or off (saved by the service)
 *   R2  held: leave
 ******************************************************************************
 */

#ifndef TRAILQUEST_TITLE_SCREEN_HPP
#define TRAILQUEST_TITLE_SCREEN_HPP

#include "gui/Sprite.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Ui.hpp"

class TitleScreen : public Screen
{
public:
    /// How long L1 is held to throw away a save for a new run, ms.
    static constexpr uint32_t kNewHoldMs = 1500;

    explicit TitleScreen(Model& model) : Screen(model) {}

    void onShow() override;
    void onWorld() override;
    void onSuspend() override;

protected:
    void build() override;
    void onKey(uint8_t code) override;
    void onFrame(uint32_t nowMs) override;

private:
    void refresh();
    void startNew();

    Sprite                mRunner;
    Sprite                mSpirit;
    Sprite                mBear;
    Ui::Corners           mCorners;
    lv_obj_t*             mStatus = nullptr;
    lv_obj_t*             mBeat   = nullptr;
    lv_obj_t*             mHint   = nullptr;
    char                  mStatusText[16] = {};
    char                  mBeatText[16]   = {};
    TrailQuest::HoldTimer mNew { kNewHoldMs };
    bool                  mStarting = false;   ///< A new game was asked for.
    uint32_t              mHintUntil = 0;      ///< "Hold for new" shows until then (0: not).
    uint32_t              mLastFlip = 0;
    uint8_t               mFrame    = 0;
};

#endif // TRAILQUEST_TITLE_SCREEN_HPP
