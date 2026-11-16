/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Pre-activity menu (see MainScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Strings.hpp"
#include "gui/WorkoutUi.hpp"
#include "PlanText.hpp"   // X Training: header-only
#include "TimeText.hpp"


#define LOG_MODULE_PRX      "MainScreen"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

using namespace SDK::GUI;

namespace
{
// Same items and geometry as MainView::setupItems() in the TouchGFX app.
using Style = WheelMenu::Item::Style;
constexpr int32_t kTipWidth = 180;   ///< X Training: the widest a hint under an item may be
const WheelMenu::Item kItems[App::MenuNav::Root::ID_COUNT] = {
    // ID_TODAY (X Training): a Tip saying what today is (onWorkoutList)
    { Style::Tip, "Today", nullptr, &poppins_semibold_35 },
    // ID_START: "Free run", or "Start" with a Tip naming the UNA workout armed
    { Style::Simple, "Free run", nullptr, &poppins_semibold_30 },
    // ID_TREADMILL (HybridX Run): on, a free run is a treadmill run (no GPS)
    { Style::Toggle, "Treadmill", nullptr, &poppins_semibold_25 },   // the intervals menu's switches' face
    // ID_WEEK, ID_PLAN, ID_CHANGES, ID_PACES (X Training, brief 8.1)
    { Style::Simple, "This week", nullptr, &poppins_semibold_30 },
    { Style::Simple, "Plan" },
    { Style::Simple, "Changes" },   // ID_CHANGES (TP5): move, skip, pause, end
    { Style::Simple, "Paces" },
    // ID_WORKOUTS: UNA's library
    { Style::Simple, "Workouts" },
    // ID_SETTINGS
    { Style::Simple, "Settings" },
};
} // namespace

MainScreen::MainScreen(Model& model)
    : Screen(model)
{
    for (uint16_t i = 0; i < Menu::ID_COUNT; ++i) {
        mItems[i] = kItems[i];
    }
}

void MainScreen::build()
{
    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    // As in MainView::onAnimationMiddle: the lens and R1 hint change half way
    // through the slide, when the incoming item is about to take the centre.
    mMenu->setSlideMidCallback(
        [](void* ctx, uint16_t) { static_cast<MainScreen*>(ctx)->updateBackground(); }, this);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, Strings::kAppNameUc);
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);

    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
}

void MainScreen::onShow()
{
    // X Training: back on the menu, today's session is no longer about to start.
    if (mModel.getArmedWorkout() == Model::kPlanSession) {
        mModel.armWorkout(Model::kNoWorkout);
    }
    mMenu->select(mModel.menu().get());
    mModel.menu().resetChildren();
    mModel.resetIdleTimer();
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
    onWorkoutList();
}

void MainScreen::onSettings(const Settings& /*settings*/)
{
    onWorkoutList();   // the switch and Start's hint follow the setting
    onGpsFix(mGpsFix);
}

void MainScreen::toggleTreadmill()
{
    Settings sett  = mModel.getSettings();
    sett.treadmill = !sett.treadmill;
    mModel.saveSettings(sett);
    onSettings(sett);
}

void MainScreen::onWorkoutList()
{
    // The armed workout's name under Start, shown when Start is selected.
    const SDK::Workout::Library* lib = mModel.getWorkoutLibrary();
    const int16_t armed = mModel.getArmedWorkout();
    mArmed = lib != nullptr && armed >= 0 && static_cast<size_t>(armed) < lib->size();
    mItems[Menu::ID_TREADMILL].toggleState = treadmill();
    setTodayTip();
    WheelMenu::Item& start = mItems[Menu::ID_START];
    if (treadmill()) {
        // HybridX Run: a plain treadmill run; an armed workout waits.
        start.text    = "Free run";
        start.style   = Style::Tip;
        start.tip     = "Treadmill";
        start.itemTip = "";
    } else if (mArmed) {
        start.text    = "Start";
        WorkoutUi::toText(lib->entry(static_cast<size_t>(armed)).name, mArmedName, sizeof(mArmedName));
        WorkoutUi::fitWidth(mArmedName, sizeof(mArmedName), Theme::font(Theme::Font::Italic18), 180);
        start.style   = Style::Tip;
        start.tip     = mArmedName;
        start.itemTip = "";
    } else {
        start.text    = "Free run";
        start.style   = Style::Simple;
        start.tip     = nullptr;
        start.itemTip = nullptr;
    }
    mMenu->refresh();
    checkTodayPrompt();
}

void MainScreen::setTodayTip()
{
    // What today is, in a line: "Easy run 40 min", "Rest day", "Starts Mon 16 Nov".
    using Info = CustomMessage::PlanTodayInfo;
    const Info& t = mModel.getPlanToday();
    switch (t.status) {
        case Info::Session: {
            // "Intervals 5 x 3 min"; the name alone, or the short name, when that is too wide
            // ("Long run, fast finish 100 min, last 20 at half pace").
            const lv_font_t* font = Theme::font(Theme::Font::Italic18);
            std::snprintf(mTodayTip, sizeof(mTodayTip), "%.31s %.31s", t.name, t.what);
            if (WorkoutUi::textWidth(mTodayTip, font) > kTipWidth) {
                std::snprintf(mTodayTip, sizeof(mTodayTip), "%.31s", t.name);
            }
            if (WorkoutUi::textWidth(mTodayTip, font) > kTipWidth) {
                std::snprintf(mTodayTip, sizeof(mTodayTip), "%s %.31s",
                              XTraining::PlanText::shortName(static_cast<XTraining::Kind>(t.kind)), t.what);
            }
        } break;
        case Info::Done:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "Done: %s",
                          XTraining::PlanText::shortName(static_cast<XTraining::Kind>(t.kind)));
            break;
        case Info::RestDay:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "Rest day");
            break;
        case Info::BeforeStart:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "Starts %.31s", t.what);
            break;
        case Info::Finished:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "Plan complete");
            break;
        case Info::Problem:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "Check your plan");
            break;
        case Info::PhoneAsks:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "New plan from phone");
            break;
        case Info::Paused:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "Plan paused");
            break;
        case Info::Offer:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "Missed: %s",
                          XTraining::PlanText::shortName(static_cast<XTraining::Kind>(t.kind)));
            break;
        case Info::PaceAsk:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "Update paces?");
            break;
        case Info::Skipped:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "Skipped: %s",
                          XTraining::PlanText::shortName(static_cast<XTraining::Kind>(t.kind)));
            break;
        case Info::NoPlan:
        default:
            std::snprintf(mTodayTip, sizeof(mTodayTip), "No plan");
            break;
    }
    WorkoutUi::fitWidth(mTodayTip, sizeof(mTodayTip), Theme::font(Theme::Font::Italic18), kTipWidth);
    WheelMenu::Item& today = mItems[Menu::ID_TODAY];
    today.style   = Style::Tip;
    today.tip     = mTodayTip;
    today.itemTip = "";
}

void MainScreen::checkTodayPrompt()
{
    if (mModel.getArmedWorkout() == Model::kNoWorkout && mModel.takeTodayPrompt()) {
        mModel.setChosenWorkout(mModel.getTodayWorkout());
        ScreenManager::instance().goTo(ScreenId::WorkoutToday);
    }
}

void MainScreen::onHide()
{
    mModel.menu().set(mMenu->selected());
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;   // lens follows at the slide midpoint
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: confirm(); break;
        case Btn::R2: mModel.exitApp(); break;
        default: break;
    }
}

void MainScreen::confirm()
{
    switch (mMenu->selected()) {
        case Menu::ID_TODAY:
            ScreenManager::instance().goTo(ScreenId::PlanToday);
            break;
        case Menu::ID_START:
            if (treadmill()) {
                // HybridX Run: indoors there is no GPS to wait for.
                mModel.trackStart(false);
                ScreenManager::instance().goTo(ScreenId::Track);
            } else if (mGpsFix && mArmed) {
                // A workout starts with the countdown, as Intervals do.
                mModel.setPendingIntervalsMode(false);
                ScreenManager::instance().goTo(ScreenId::TrackIntervalsCountdown);
            } else if (mGpsFix) {
                mModel.trackStart(false);
                ScreenManager::instance().goTo(ScreenId::Track);
            } else {
                mModel.setPendingIntervalsMode(false);
                ScreenManager::instance().goTo(ScreenId::TrackStartConfirm);
            }
            break;
        case Menu::ID_TREADMILL:
            toggleTreadmill();
            break;
        case Menu::ID_WEEK:
            ScreenManager::instance().goTo(ScreenId::PlanWeek);
            break;
        case Menu::ID_PLAN:
            ScreenManager::instance().goTo(ScreenId::PlanOverview);
            break;
        case Menu::ID_CHANGES:
            ScreenManager::instance().goTo(ScreenId::PlanChanges);
            break;
        case Menu::ID_PACES:
            ScreenManager::instance().goTo(ScreenId::PlanPaces);
            break;
        case Menu::ID_WORKOUTS:
            ScreenManager::instance().goTo(ScreenId::WorkoutList);
            break;
        case Menu::ID_SETTINGS:
            ScreenManager::instance().goTo(ScreenId::MenuSettings);
            break;
        default:
            break;
    }
}

void MainScreen::updateBackground()
{
    // Start without a fix is greyed out and R1 hidden; everything else is live.
    // HybridX Run: a treadmill run needs no fix.
    const bool startBlocked = mMenu->selected() == Menu::ID_START && !mGpsFix && !treadmill();
    mMenu->setBackground(startBlocked ? Color::GRAY_DARK : Color::TEAL_DARK);
    mButtons->setR1(startBlocked ? Widgets::Buttons::NONE : Widgets::Buttons::AMBER);
}

void MainScreen::onIdleTimeout()
{
    if (mMenu->selected() != Menu::ID_START) {
        mModel.exitApp();
    }
}

void MainScreen::onGpsFix(bool acquired)
{
    mGpsFix = acquired;
    mSensorRow->setGps(treadmill() ? Widgets::SensorStatusRow::State::Absent
                                   : Widgets::SensorStatusRow::gpsState(acquired));
    updateBackground();
}

void MainScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
