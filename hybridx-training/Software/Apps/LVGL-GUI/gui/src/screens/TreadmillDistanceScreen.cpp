/**
 ******************************************************************************
 * @file    TreadmillDistanceScreen.cpp
 * @brief   HybridX Run: the treadmill's distance after a run (see the header).
 ******************************************************************************
 */

#include "gui/screens/TreadmillDistanceScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include <cstdio>

#include "SDK/Calibration/CadenceStrideModelConfig.hpp"

using namespace SDK::GUI;
using F = Theme::Font;
namespace TM = RunVo2::Treadmill;

TreadmillDistanceScreen::TreadmillDistanceScreen(Model& model)
    : Screen(model)
{
}

void TreadmillDistanceScreen::build()
{
    // The VO2 pickers' faces and rows: the value, then the next two above it.
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mSub   = Theme::label(mRoot, F::Italic20, "", 20, 58, 200, LV_TEXT_ALIGN_CENTER, Color::TEAL);
    mVal   = Theme::label(mRoot, F::SemiBold60, "", 20, 92, 200, LV_TEXT_ALIGN_CENTER, Color::TEAL);
    mNext1 = Theme::label(mRoot, F::Medium40, "", 20, 151, 200, LV_TEXT_ALIGN_CENTER, Color::WHITE);
    // One row above the value (not two, as the VO2 pickers): the second row's place holds
    // the hint, clear of the title and inside the round face.
    mHint  = Theme::label(mRoot, F::Regular16, "", 40, 198, 160, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "TREADMILL");
}

void TreadmillDistanceScreen::onShow()
{
    mImperial = mModel.isUnitsImperial();
    seed(mModel.treadmillEstimateM());
    mFine     = false;
    mTouched  = false;
    render();
    mModel.resetIdleTimer();
}

void TreadmillDistanceScreen::onTreadmillAsk(float estimateM)
{
    // The estimate came after the screen opened: offer it, unless the value was changed by hand.
    if (!mTouched) {
        seed(estimateM);
        render();
    }
}

void TreadmillDistanceScreen::seed(float estimateM)
{
    namespace Cfg = SDK::Calibration::Config;
    mRange = TM::range(estimateM, mImperial, Cfg::kDActualRatioMin, Cfg::kDActualRatioMax);
    mValue = TM::step(TM::toHundredths(estimateM, mImperial), 0, mRange);
}

void TreadmillDistanceScreen::render()
{
    char a[8], up1[8] = "";
    TM::text(a, sizeof(a), mValue);
    const uint16_t n1 = TM::step(mValue, stepSize(), mRange);
    if (n1 != mValue) TM::text(up1, sizeof(up1), n1);
    lv_label_set_text(mVal, a);
    lv_label_set_text(mNext1, up1);

    const char* unit = mImperial ? "mi" : "km";
    char sub[32];
    std::snprintf(sub, sizeof(sub), mFine ? "%s, fine" : "Console %s", unit);
    lv_label_set_text(mSub, sub);
    lv_label_set_text(mHint, mFine ? "R1 saves" : "R2 keeps estimate");
}

void TreadmillDistanceScreen::finish(bool useEntry)
{
    if (mDone) {
        return;
    }
    mDone = true;
    // 0 keeps the estimate. An entry equal to the estimate's rounding is still sent: the
    // athlete has confirmed it, and the service keeps the stride as it is for it.
    mModel.sendTreadmillDistance(useEntry ? TM::toMetres(mValue, mImperial) : 0.0f);
    ScreenManager::instance().goTo(ScreenId::TrackSummary);
}

void TreadmillDistanceScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1:
            mValue   = TM::step(mValue, -stepSize(), mRange);
            mTouched = true;
            render();
            break;
        case Btn::L2:
            mValue   = TM::step(mValue, stepSize(), mRange);
            mTouched = true;
            render();
            break;
        case Btn::R1:
            if (!mFine) {
                mFine = true;   // first press: on to the hundredths
                render();
            } else {
                finish(true);
            }
            break;
        case Btn::R2:
            if (mFine) {
                mFine = false;  // back to the tenths
                render();
            } else {
                finish(false);  // Skip: keep the estimate
            }
            break;
        default:
            break;
    }
    mModel.resetIdleTimer();
}
