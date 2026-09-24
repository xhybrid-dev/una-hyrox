/**
 ******************************************************************************
 * @file    SummitScreen.cpp
 * @brief   A summit reached (see the header).
 ******************************************************************************
 */

#include "gui/screens/SummitScreen.hpp"

#include <cstdio>

#include "Summits.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{
constexpr int16_t kHeroBaseY = 176;
constexpr int16_t kHeroScale = 120;

/// Confetti: where each piece falls, when it starts, how long it takes.
struct Piece {
    int16_t  x;
    int16_t  startY;
    uint16_t delayMs;
    uint16_t fallMs;
    uint8_t  colour;
};
constexpr uint32_t kConfettiColours[] = { SDK::GUI::Color::LIME, SDK::GUI::Color::CYAN, SDK::GUI::Color::LEMON,
                                          SDK::GUI::Color::WHITE };
// A fixed scatter rather than rand(): the same celebration every time, and
// reproducible screenshots.
constexpr Piece kPieces[] = {
    {  46, -10,   0, 2200, 0 }, {  70, -40, 150, 2600, 1 }, {  94, -20, 300, 2000, 2 }, { 118, -60,  60, 2400, 3 },
    { 142, -15, 420, 2100, 0 }, { 166, -45, 210, 2500, 1 }, { 190, -25, 360, 2300, 2 }, {  58, -70, 520, 2700, 3 },
    {  82, -30, 640, 2200, 1 }, { 106, -50, 780, 2600, 0 }, { 130, -35, 900, 2000, 3 }, { 154, -65, 700, 2400, 2 },
    { 178, -20, 580, 2150, 1 }, { 202, -55, 820, 2550, 0 }, {  34, -45, 960, 2300, 2 }, { 214, -30,1040, 2250, 3 },
};
} // namespace

SummitScreen::SummitScreen(Model& model)
    : Screen(model)
{
}

SummitScreen::~SummitScreen() = default;

void SummitScreen::build()
{
    const uint8_t        climb = mModel.summited();
    const Streak::Climb& c     = Streak::kClimbs[climb < Streak::kClimbCount ? climb : Streak::kClimbCount - 1];

    SummitScene::Options opts;
    opts.baseY        = kHeroBaseY;
    opts.scalePercent = kHeroScale;
    opts.drawFlag     = false;
    mScene = std::make_unique<SummitScene>(mRoot, opts);
    mScene->setProgress(climb, c.steps, c.steps);   // the whole trail climbed
    mFlag = std::make_unique<Widgets::Flag>(mRoot, mScene->apex(), 22);

    confetti();

    Theme::label(mRoot, Theme::Font::SemiBold30, "Summit!", 20, 14, 200, LV_TEXT_ALIGN_CENTER, Palette::kText);

    char buf[48];
    snprintf(buf, sizeof(buf), "%s \xC2\xB7 %u weeks", c.name, static_cast<unsigned>(c.summitAt));
    Theme::label(mRoot, Theme::Font::Medium18, buf, 10, kHeroBaseY + 4, 220, LV_TEXT_ALIGN_CENTER, Palette::kWin);

    const uint8_t next = static_cast<uint8_t>(climb + 1);
    if (next < Streak::kClimbCount) {
        snprintf(buf, sizeof(buf), "Next: %s", Streak::kClimbs[next].name);
    } else {
        snprintf(buf, sizeof(buf), "Next: Everest again");
    }
    Theme::label(mRoot, Theme::Font::Regular14, buf, 40, kHeroBaseY + 28, 160, LV_TEXT_ALIGN_CENTER,
                 Palette::kTextSoft);

    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::AMBER, Widgets::Buttons::NONE);
}

void SummitScreen::confetti()
{
    for (const Piece& p : kPieces) {
        lv_obj_t* bit = Theme::box(mRoot, p.x, p.startY, 4, 7, kConfettiColours[p.colour]);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, bit);
        lv_anim_set_values(&a, p.startY, 250);
        lv_anim_set_delay(&a, p.delayMs);
        lv_anim_set_duration(&a, p.fallMs);
        lv_anim_set_repeat_count(&a, 2);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
        lv_anim_set_exec_cb(&a, reinterpret_cast<lv_anim_exec_xcb_t>(lv_obj_set_y));
        lv_anim_start(&a);   // LVGL deletes an object's animations with the object
    }
}

void SummitScreen::onShow()
{
    mModel.resetIdleTimer();
    mModel.celebrate(CustomMessage::Moment::Summit);
}

void SummitScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code == Btn::R1 || code == Btn::R2) {
        ScreenManager::instance().goTo(ScreenId::Home);
        return;
    }
#if HYBRIDXSTREAK_DEMO
    if (code == Btn::L1 || code == Btn::L2) {
        ScreenManager::instance().goTo(mModel.demoGo(code == Btn::L1 ? -1 : 1));
    }
#endif
}
