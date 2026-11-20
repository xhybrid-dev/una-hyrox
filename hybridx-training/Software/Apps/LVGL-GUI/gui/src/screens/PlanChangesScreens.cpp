/**
 ******************************************************************************
 * @file    PlanChangesScreens.cpp
 * @brief   The plan's changes and the answers to its offers (see the header).
 ******************************************************************************
 */

#include "gui/screens/PlanChangesScreens.hpp"

#include <cstdio>

#include "gui/WorkoutUi.hpp"
#include "gui/model/Model.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using Info = CustomMessage::PlanTodayInfo;
using Act = CustomMessage::PlanAction;
using Style = WheelMenu::Item::Style;
namespace Color = SDK::GUI::Color;

// -- Changes -----------------------------------------------------------------------------

PlanChangesScreen::PlanChangesScreen(Model& model) : Screen(model) {}
PlanChangesScreen::~PlanChangesScreen() = default;

void PlanChangesScreen::build()
{
    mItems[Menu::ID_TOMORROW_TODAY] = { Style::Tip, "Tomorrow's" };   // do tomorrow's session today
    mItems[Menu::ID_TODAY_TOMORROW] = { Style::Tip, "Today's" };      // do today's session tomorrow
    mItems[Menu::ID_SKIP]           = { Style::Tip, "Skip today" };
    mItems[Menu::ID_PAUSE]          = { Style::Tip, "Pause plan" };
    mItems[Menu::ID_END]            = { Style::Tip, "End plan" };
    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mMenu->setCircular(false);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "CHANGES");
}

bool PlanChangesScreen::planUnderWay() const
{
    const uint8_t s = mModel.getPlanToday().status;
    return s == Info::Session || s == Info::RestDay || s == Info::Done || s == Info::Paused
        || s == Info::Offer || s == Info::PaceAsk || s == Info::Skipped;
}

void PlanChangesScreen::onShow()
{
    mModel.resetIdleTimer();
    mConfirmEnd = false;
    mMenu->select(0);
    refresh();
}

void PlanChangesScreen::refresh()
{
    const bool paused = mModel.getPlanToday().status == Info::Paused;
    mItems[Menu::ID_TOMORROW_TODAY].tip = "do it today";
    mItems[Menu::ID_TODAY_TOMORROW].tip = "do it tomorrow";
    mItems[Menu::ID_SKIP].tip           = "not today";
    mItems[Menu::ID_PAUSE].text         = paused ? "Resume plan" : "Pause plan";
    mItems[Menu::ID_PAUSE].tip          = paused ? "carry on" : "nothing missed";
    mItems[Menu::ID_END].tip            = mConfirmEnd ? "R1 again to end" : "stops the plan";
    for (uint16_t i = 0; i < Menu::ID_COUNT; ++i) {
        mItems[i].itemTip = "";
    }
    if (!planUnderWay()) {
        mItems[Menu::ID_TOMORROW_TODAY].tip = mItems[Menu::ID_TODAY_TOMORROW].tip =
            mItems[Menu::ID_SKIP].tip = mItems[Menu::ID_PAUSE].tip = mItems[Menu::ID_END].tip = "no plan under way";
    }
    mMenu->setBackground(Color::TEAL_DARK);
    mMenu->refresh();
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  planUnderWay() ? Widgets::Buttons::AMBER : Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
}

void PlanChangesScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mConfirmEnd = false; mMenu->prev(); refresh(); break;
        case Btn::L2: mConfirmEnd = false; mMenu->next(); refresh(); break;
        case Btn::R1: {
            if (!planUnderWay()) {
                break;
            }
            const uint16_t sel = mMenu->selected();
            if (sel == Menu::ID_END) {
                if (!mConfirmEnd) {
                    mConfirmEnd = true;   // ending a plan is not undone: ask again
                    refresh();
                    break;
                }
                mModel.planAction(Act::EndPlan);
                ScreenManager::instance().goTo(ScreenId::Main);
                break;
            }
            const bool paused = mModel.getPlanToday().status == Info::Paused;
            switch (sel) {
                case Menu::ID_TOMORROW_TODAY: mModel.planAction(Act::DoTomorrow); break;
                case Menu::ID_TODAY_TOMORROW: mModel.planAction(Act::MoveTomorrow); break;
                case Menu::ID_SKIP:           mModel.planAction(Act::Skip); break;
                case Menu::ID_PAUSE:          mModel.planAction(paused ? Act::Resume : Act::Pause); break;
                default: break;
            }
            ScreenManager::instance().goTo(ScreenId::PlanToday);   // the answer's title says how it went
        } break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Main);
            break;
        default: break;
    }
}

// -- An offer, or a trial's question -----------------------------------------------------

PlanOfferScreen::PlanOfferScreen(Model& model) : Screen(model) {}
PlanOfferScreen::~PlanOfferScreen() = default;

bool PlanOfferScreen::pace() const
{
    return mModel.getPlanToday().status == Info::PaceAsk;
}

void PlanOfferScreen::build()
{
    for (uint16_t i = 0; i < ID_COUNT; ++i) {
        mItems[i] = { Style::Tip, mText[i] };
        mItems[i].tip = mTip[i];
        mItems[i].itemTip = "";
    }
    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, ID_COUNT);
    mMenu->setCircular(false);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::WHITE, Widgets::Buttons::WHITE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, mTitleText);
}

void PlanOfferScreen::onShow()
{
    mModel.resetIdleTimer();
    const Info& t = mModel.getPlanToday();
    if (pace()) {
        std::snprintf(mTitleText, sizeof(mTitleText), "YOUR PACES?");
        std::snprintf(mText[ID_YES], sizeof(mText[0]), "Update");
        std::snprintf(mText[ID_NO], sizeof(mText[0]), "Keep mine");
        std::snprintf(mTip[ID_YES], sizeof(mTip[0]), "5K %u:%02u", static_cast<unsigned>(t.plannedSec / 60),
                      static_cast<unsigned>(t.plannedSec % 60));
        std::snprintf(mTip[ID_NO], sizeof(mTip[0]), "5K %u:%02u", static_cast<unsigned>(t.plannedM / 60),
                      static_cast<unsigned>(t.plannedM % 60));
    } else {
        std::snprintf(mTitleText, sizeof(mTitleText), t.detail == Info::LongRun ? "DO IT TODAY?" : "DO IT INSTEAD?");
        std::snprintf(mText[ID_YES], sizeof(mText[0]), "Yes, today");
        std::snprintf(mText[ID_NO], sizeof(mText[0]), "No, drop it");
        std::snprintf(mTip[ID_YES], sizeof(mTip[0]), "%.30s", t.name);
        std::snprintf(mTip[ID_NO], sizeof(mTip[0]), "not offered again");
    }
    mTitle->setText(mTitleText);
    mMenu->select(0);
    mMenu->setBackground(Color::TEAL_DARK);
    mMenu->refresh();
}

void PlanOfferScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: {
            const bool yes = mMenu->selected() == ID_YES;
            if (pace()) {
                mModel.planAction(yes ? Act::AnswerUpdate : Act::AnswerKeep);
            } else {
                mModel.planAction(yes ? Act::AcceptOffer : Act::DeclineOffer);
            }
            ScreenManager::instance().goTo(ScreenId::PlanToday);
        } break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::PlanToday);
            break;
        default: break;
    }
}
