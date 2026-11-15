/**
 ******************************************************************************
 * @file    Vo2PickerScreen.cpp
 * @brief   HybridX Run: pickers for the athlete's numbers (see the header).
 ******************************************************************************
 */

#include "gui/screens/Vo2PickerScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

#include <cstdio>

#include "Vo2Settings.hpp"

using namespace SDK::GUI;
using F = Theme::Font;
namespace Set = RunVo2::Settings;

Vo2PickerScreen::Vo2PickerScreen(Model& model, Kind kind)
    : Screen(model)
    , mKind(kind)
{
}

void Vo2PickerScreen::build()
{
    // One value, centred, in the TwoTonePicker's faces and rows.
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mSub   = Theme::label(mRoot, F::Italic20, "", 20, 58, 200, LV_TEXT_ALIGN_CENTER, Color::TEAL);
    mVal   = Theme::label(mRoot, F::SemiBold60, "", 20, 92, 200, LV_TEXT_ALIGN_CENTER, Color::TEAL);
    mNext1 = Theme::label(mRoot, F::Medium40, "", 20, 151, 200, LV_TEXT_ALIGN_CENTER, Color::WHITE);
    mNext2 = Theme::label(mRoot, F::Medium25, "", 20, 193, 200, LV_TEXT_ALIGN_CENTER, Color::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, mKind == Kind::Birth   ? "BIRTH DATE"
                                                     : mKind == Kind::MaxHr ? "MAX HR"
                                                                            : "RESTING HR");
}

uint8_t Vo2PickerScreen::inUse() const
{
    const CustomMessage::Vo2Info& v = mModel.getVo2Info();
    return mKind == Kind::MaxHr ? v.nowMaxHr : v.nowRestHr;
}

void Vo2PickerScreen::onShow()
{
    const CustomMessage::Vo2Info& v = mModel.getVo2Info();
    mYear  = v.birthYear;
    mMonth = v.birthMonth;
    Set::seedBirth(mYear, mMonth);
    mAtYear = false;
    mValue  = mKind == Kind::MaxHr ? v.setMaxHr : v.setRestHr;
    render();
    mModel.resetIdleTimer();
}

void Vo2PickerScreen::render()
{
    char a[8], up1[8] = "", up2[8] = "";
    if (mKind == Kind::Birth) {
        if (!mAtYear) {
            const uint8_t m1 = Set::stepMonth(mMonth, +1), m2 = Set::stepMonth(m1, +1);
            std::snprintf(a, sizeof(a), "%02u", mMonth);
            if (m1 != mMonth) std::snprintf(up1, sizeof(up1), "%02u", m1);
            if (m2 != m1)     std::snprintf(up2, sizeof(up2), "%02u", m2);
            lv_label_set_text(mSub, "Month");
        } else {
            const uint16_t y1 = Set::stepYear(mYear, +1), y2 = Set::stepYear(y1, +1);
            std::snprintf(a, sizeof(a), "%u", mYear);
            if (y1 != mYear) std::snprintf(up1, sizeof(up1), "%u", y1);
            if (y2 != y1)    std::snprintf(up2, sizeof(up2), "%u", y2);
            lv_label_set_text(mSub, "Year");
        }
        lv_label_set_text(mVal, a);
        lv_label_set_text(mNext1, up1);
        lv_label_set_text(mNext2, up2);
        return;
    }

    auto stepOf = [this](uint8_t v) {
        return mKind == Kind::MaxHr ? Set::stepMaxHr(v, +1, inUse()) : Set::stepRestHr(v, +1, inUse());
    };
    auto show = [](uint8_t v, char* out, size_t cap) {
        if (v == 0) {
            std::snprintf(out, cap, "---");
        } else {
            std::snprintf(out, cap, "%u", v);
        }
    };
    show(mValue, a, sizeof(a));
    lv_label_set_text(mVal, a);
    const uint8_t n1 = stepOf(mValue);
    const uint8_t n2 = stepOf(n1);
    if (n1 != mValue) show(n1, up1, sizeof(up1));
    if (n2 != n1)     show(n2, up2, sizeof(up2));
    lv_label_set_text(mNext1, up1);
    lv_label_set_text(mNext2, up2);

    if (mValue != 0) {
        lv_label_set_text(mSub, "bpm");
    } else if (mKind == Kind::MaxHr) {
        lv_label_set_text(mSub, "Auto: from age");
    } else {
        lv_label_set_text(mSub, "From the watch");
    }
}

void Vo2PickerScreen::step(int dir)
{
    switch (mKind) {
        case Kind::Birth:
            if (mAtYear) {
                mYear = Set::stepYear(mYear, dir);
            } else {
                mMonth = Set::stepMonth(mMonth, dir);
            }
            break;
        case Kind::MaxHr:  mValue = Set::stepMaxHr(mValue, dir, inUse()); break;
        case Kind::RestHr: mValue = Set::stepRestHr(mValue, dir, inUse()); break;
    }
    render();
}

void Vo2PickerScreen::save()
{
    const CustomMessage::Vo2Info& v = mModel.getVo2Info();
    switch (mKind) {
        case Kind::Birth:
            mModel.saveVo2Profile(mYear, mMonth, v.setMaxHr, v.setRestHr, v.tuningLog);
            break;
        case Kind::MaxHr:
            mModel.saveVo2Profile(v.birthYear, v.birthMonth, mValue, v.setRestHr, v.tuningLog);
            break;
        case Kind::RestHr:
            mModel.saveVo2Profile(v.birthYear, v.birthMonth, v.setMaxHr, mValue, v.tuningLog);
            break;
    }
}

void Vo2PickerScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: step(-1); break;
        case Btn::L2: step(+1); break;
        case Btn::R1:
            if (mKind == Kind::Birth && !mAtYear) {
                mAtYear = true;   // first press: on to the year
                render();
            } else {
                save();
                ScreenManager::instance().goTo(ScreenId::MenuVo2Profile);
            }
            break;
        case Btn::R2:
            if (mKind == Kind::Birth && mAtYear) {
                mAtYear = false;  // back to the month
                render();
            } else {
                ScreenManager::instance().goTo(ScreenId::MenuVo2Profile);
            }
            break;
        default:
            break;
    }
    mModel.resetIdleTimer();
}
