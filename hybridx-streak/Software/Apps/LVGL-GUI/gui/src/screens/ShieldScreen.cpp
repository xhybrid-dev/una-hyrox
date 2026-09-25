/**
 ******************************************************************************
 * @file    ShieldScreen.cpp
 * @brief   A week missed with a shield in hand (see the header).
 ******************************************************************************
 */

#include "gui/screens/ShieldScreen.hpp"

#include <cstdio>

#include "gui/Assets.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{
constexpr uint32_t kSavedMs = 2200;   ///< "Streak saved." before going home

void shieldsLeft(char* buf, size_t size, unsigned n)
{
    snprintf(buf, size, "%u shield%s left", n, n == 1 ? "" : "s");
}
} // namespace

ShieldScreen::ShieldScreen(Model& model)
    : Screen(model)
{
}

ShieldScreen::~ShieldScreen()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
}

void ShieldScreen::build()
{
    const Streak::HomeView& v = mModel.home();

    Widgets::shieldGlyph(mRoot, 120, 18);

    mTitle = Theme::label(mRoot, Theme::Font::SemiBold20, "Life happens.", 20, 96, 200, LV_TEXT_ALIGN_CENTER,
                          Palette::kText);

    char buf[96];
    // Worded so no number is split from its unit at a line break: LVGL breaks
    // lines at hyphens, and "9-" / "week" reads badly. The body stays clear of
    // the cross beside R2 (x 187).
#if HYBRIDXSTREAK_DEMO
    const unsigned missed = 1;
    const unsigned streak = v.streakWeeks;
#else
    // The service's offer: how many weeks were missed, and the streak at stake.
    const unsigned missed = mModel.offer().a > 0 ? mModel.offer().a : 1;
    const unsigned streak = mModel.offer().b;
#endif
    if (missed == 1) {
        snprintf(buf, sizeof(buf), "%u of %u last week. Use a shield to keep your streak of %u weeks?",
                 static_cast<unsigned>(v.lastWeek), static_cast<unsigned>(v.target), streak);
    } else {
        snprintf(buf, sizeof(buf), "%u weeks missed. Use %u shields to keep your streak of %u weeks?", missed,
                 missed, streak);
    }
    mBody = Theme::label(mRoot, Theme::Font::Regular16, buf, 40, 122, 140, LV_TEXT_ALIGN_CENTER, Palette::kTextSoft);
    lv_label_set_long_mode(mBody, LV_LABEL_LONG_WRAP);

    shieldsLeft(buf, sizeof(buf), v.shields);
    mCount = Theme::label(mRoot, Theme::Font::Regular14, buf, 50, 202, 140, LV_TEXT_ALIGN_CENTER, Palette::kShield);

    // UNA's convention: a tick beside R1 to confirm, a cross beside R2 to decline.
    mTick  = Theme::imageTinted(mRoot, &img_tickgreen_22x17, 186, 60, SDK::GUI::Color::CHARTREUSE);
    mCross = Theme::imageTinted(mRoot, &img_crosswhite_17x17, 187, 163, SDK::GUI::Color::WHITE);

    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::GREEN, Widgets::Buttons::WHITE);
}

void ShieldScreen::onShow()
{
    mModel.resetIdleTimer();
}

void ShieldScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (mSpent) {
        return;
    }
    if (code == Btn::R1) {
        spend();
        return;
    }
    if (code == Btn::R2) {
#if HYBRIDXSTREAK_DEMO
        Streak::HomeView v = mModel.home();
        v.streakWeeks      = 0;
        mModel.setHome(v);
#else
        mModel.setLostStreak(mModel.offer().b);
        mModel.decideShields(false);
#endif
        ScreenManager::instance().goTo(ScreenId::FreshStart);
        return;
    }
#if HYBRIDXSTREAK_DEMO
    if (code == Btn::L1 || code == Btn::L2) {
        ScreenManager::instance().goTo(mModel.demoGo(code == Btn::L1 ? -1 : 1));
    }
#endif
}

void ShieldScreen::spend()
{
    mSpent = true;
    mModel.celebrate(CustomMessage::Moment::Shield);

    const Streak::HomeView& v = mModel.home();
#if HYBRIDXSTREAK_DEMO
    const unsigned streak = v.streakWeeks;
    const unsigned used   = 1;
#else
    const unsigned streak = mModel.offer().b;
    const unsigned used   = mModel.offer().a > 0 ? mModel.offer().a : 1;
    mModel.decideShields(true);
#endif
    char buf[96];
    lv_label_set_text(mTitle, "Streak saved.");
    Theme::setColor(mTitle, Palette::kWin);
    snprintf(buf, sizeof(buf), "Your streak of %u weeks climbs on. Rest well.", streak);
    lv_label_set_text(mBody, buf);
    shieldsLeft(buf, sizeof(buf), v.shields > used ? v.shields - used : 0u);
    lv_label_set_text(mCount, buf);
    Theme::setHidden(mTick, true);
    Theme::setHidden(mCross, true);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::NONE);

    mTimer = lv_timer_create(&ShieldScreen::doneCb, kSavedMs, this);
    lv_timer_set_repeat_count(mTimer, 1);
}

void ShieldScreen::doneCb(lv_timer_t* t)
{
    auto* self   = static_cast<ShieldScreen*>(lv_timer_get_user_data(t));
    self->mTimer = nullptr;

#if HYBRIDXSTREAK_DEMO
    // A new week: the streak held, one shield fewer, nothing logged yet.
    Streak::HomeView v = self->mModel.home();
    v.shields   = v.shields > 0 ? static_cast<uint8_t>(v.shields - 1) : 0;
    v.sessions  = 0;
    v.daysLeft  = 7;
    v.mood      = Streak::Mood::Climbing;
    self->mModel.setHome(v);
#endif
    // For real, the service has sent the new view already.
    ScreenManager::instance().goTo(ScreenId::Home);
}
