/**
 ******************************************************************************
 * @file    FreshStartScreen.cpp
 * @brief   The streak ended, and the climb is safe (see the header).
 ******************************************************************************
 */

#include "gui/screens/FreshStartScreen.hpp"

#include <cstdio>

#include "Summits.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

FreshStartScreen::FreshStartScreen(Model& model)
    : Screen(model)
{
}

FreshStartScreen::~FreshStartScreen() = default;

void FreshStartScreen::build()
{
    const Streak::HomeView&     v   = mModel.home();
    const Streak::ClimbPosition pos = Streak::climbFor(v.weeksAchieved);

    mSunrise = std::make_unique<Widgets::Sunrise>(mRoot);

    Theme::label(mRoot, Theme::Font::SemiBold25, "Fresh start.", 20, 122, 200, LV_TEXT_ALIGN_CENTER, Palette::kText);

    char buf[96];
    snprintf(buf, sizeof(buf), "Your climb is safe: %u of %u on %s. A new streak starts today.",
             static_cast<unsigned>(pos.stepsClimbed), static_cast<unsigned>(pos.steps),
             Streak::kClimbs[pos.climb].name);
    lv_obj_t* body = Theme::label(mRoot, Theme::Font::Regular14, buf, 36, 156, 168, LV_TEXT_ALIGN_CENTER,
                                  Palette::kTextSoft);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);

    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::AMBER, Widgets::Buttons::NONE);
}

void FreshStartScreen::onShow()
{
    mModel.resetIdleTimer();
}

void FreshStartScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code == Btn::R1) {
        Streak::HomeView v = mModel.home();
        v.streakWeeks = 0;
        v.sessions    = 0;
        v.daysLeft    = 7;
        v.mood        = Streak::Mood::Climbing;
        mModel.setHome(v);
        ScreenManager::instance().goTo(ScreenId::Home);
        return;
    }
#if HYBRIDXSTREAK_DEMO
    if (code == Btn::L1 || code == Btn::L2) {
        ScreenManager::instance().goTo(mModel.demoGo(code == Btn::L1 ? -1 : 1));
    }
#endif
}
