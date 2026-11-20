/**
 ******************************************************************************
 * @file    FeelScreen.cpp
 * @brief   How the runner felt after a planned session (see the header).
 ******************************************************************************
 */

#include "gui/screens/FeelScreen.hpp"

#include "gui/model/Model.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Style = WheelMenu::Item::Style;
namespace Color = SDK::GUI::Color;

FeelScreen::FeelScreen(Model& model) : Screen(model) {}
FeelScreen::~FeelScreen() = default;

void FeelScreen::build()
{
    // Words and a few of their own, in the order of Review::Feel.
    mItems[ID_FRESH]  = { Style::Tip, "Fresh" };    mItems[ID_FRESH].tip  = "ready for more";
    mItems[ID_GOOD]   = { Style::Tip, "Good" };     mItems[ID_GOOD].tip   = "felt strong";
    mItems[ID_STEADY] = { Style::Tip, "Steady" };   mItems[ID_STEADY].tip = "an ordinary day";
    mItems[ID_TIRED]  = { Style::Tip, "Tired" };    mItems[ID_TIRED].tip  = "legs were heavy";
    mItems[ID_SPENT]  = { Style::Tip, "Spent" };    mItems[ID_SPENT].tip  = "nothing left";
    mItems[ID_SKIP]   = { Style::Tip, "Skip" };     mItems[ID_SKIP].tip   = "no answer";
    for (uint16_t i = 0; i < ID_COUNT; ++i) {
        mItems[i].itemTip = "";
    }
    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, ID_COUNT);
    mMenu->setCircular(false);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "HOW IT FELT");
}

void FeelScreen::onShow()
{
    mModel.resetIdleTimer();
    mMenu->select(ID_STEADY);   // the middle: most days are
    mMenu->setBackground(Color::TEAL_DARK);
    mMenu->refresh();
}

void FeelScreen::answer(uint8_t feel)
{
    if (feel != 0) {
        mModel.planAction(CustomMessage::PlanAction::Feel, feel);
    }
    // On to what the save flow had next: a treadmill's distance, else the summary.
    ScreenManager::instance().goTo(mModel.getTrackData().treadmill ? ScreenId::TreadmillDistance
                                                                  : ScreenId::TrackSummary);
}

void FeelScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    mModel.resetIdleTimer();
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: {
            const uint16_t sel = mMenu->selected();
            answer(sel == ID_SKIP ? 0 : static_cast<uint8_t>(sel + 1));
        } break;
        case Btn::R2:
            answer(0);
            break;
        default: break;
    }
}
