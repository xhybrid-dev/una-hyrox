/**
 ******************************************************************************
 * @file    TreadmillDistanceScreen.hpp
 * @brief   HybridX Run: after a treadmill run is saved, the distance the
 *          treadmill's console shows (NOTES, "Treadmill").
 *
 * The estimate (from cadence) is offered first, in km or miles by the watch's
 * units, to two decimals. Two stages, as the birth-date picker: L1 and L2 step
 * by 0.10, R1 moves on to step by 0.01, R1 again saves. The next value shows
 * above, and the keys' meaning below. R2 on the first stage
 * keeps the estimate (Skip); on the second it goes back. Either way the
 * summary follows. The service corrects the session with the entry and learns
 * the treadmill stride from it. The picker stops at the band the SDK's stride
 * model accepts (half to twice the estimate), so no entry is silently dropped.
 * Steps and range: RunVo2::Treadmill (host-tested).
 ******************************************************************************
 */

#ifndef TREADMILL_DISTANCE_SCREEN_HPP
#define TREADMILL_DISTANCE_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

#include "Treadmill.hpp"

class TreadmillDistanceScreen : public Screen
{
public:
    explicit TreadmillDistanceScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onTreadmillAsk(float estimateM) override;
    /// Left alone, the run keeps its estimate and the summary follows.
    void onIdleTimeout() override { finish(false); }

protected:
    void build() override;

private:
    void render();
    void seed(float estimateM);   ///< the estimate offered, and the range around it
    void finish(bool useEntry);
    int  stepSize() const { return mFine ? 1 : 10; }

    bool     mImperial = false;
    bool     mFine     = false;   ///< the second stage: hundredths
    bool     mTouched  = false;   ///< changed by hand: a late estimate does not overwrite it
    bool     mDone     = false;
    uint16_t mValue    = 0;       ///< hundredths of a km or mile
    RunVo2::Treadmill::Range mRange {};   ///< what the stride model will accept

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    lv_obj_t* mSub   = nullptr;
    lv_obj_t* mVal   = nullptr;
    lv_obj_t* mNext1 = nullptr;
    lv_obj_t* mHint  = nullptr;
};

#endif // TREADMILL_DISTANCE_SCREEN_HPP
