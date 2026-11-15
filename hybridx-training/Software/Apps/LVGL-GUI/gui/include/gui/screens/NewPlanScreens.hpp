/**
 ******************************************************************************
 * @file    NewPlanScreens.hpp
 * @brief   X Training (TP3): a new plan made on the watch (brief 5.2), and the
 *          answer to a new plan from the phone (brief 5.1).
 *
 * New plan (from Plan, R1) is a wheel of the choices, each with its value as
 * the hint: the plan, the 5K (a time picker), whether it is recent, the level,
 * the run days (seven switches), the long run day, the race date (a list of
 * weekends), then Next: the new plan's summary and calendar, R1 to start it.
 * The draft is the Model's, seeded from the phone's set-up. Starting writes the
 * set-up to the phone's values file, so the phone shows what the watch runs.
 *
 * When the phone's settings ask for a new plan in place of one under way, Today
 * asks; R1 there opens PlanAskScreen: start it, or keep yours (which writes
 * yours back to the phone's values).
 ******************************************************************************
 */

#ifndef NEW_PLAN_SCREENS_HPP
#define NEW_PLAN_SCREENS_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/PickerLogic.hpp"
#include "gui/widgets/TextStack.hpp"
#include "gui/widgets/WheelMenu.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/model/AppMenu.hpp"

/// The choices.
class NewPlanScreen : public Screen
{
public:
    explicit NewPlanScreen(Model& model);
    ~NewPlanScreen() override;
    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;
    void onPlanSetup() override;

protected:
    void build() override;

private:
    using Menu = App::MenuNav::NewPlanMenu;
    void refresh();
    void confirm();

    static uint16_t sPos;   ///< the wheel's place, kept while a picker is open
    WheelMenu::Item mItems[Menu::ID_COUNT] {};
    char mTip[Menu::ID_COUNT][40] = {};
    std::unique_ptr<WheelMenu>        mMenu;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

/// The 5K time, minutes then seconds.
class FiveKPickerScreen : public Screen
{
public:
    explicit FiveKPickerScreen(Model& model);
    ~FiveKPickerScreen() override;
    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    struct Range {
        static constexpr uint16_t kMaxMin  = 59;
        static constexpr uint16_t kStepMin = 1;
        static constexpr uint16_t kMaxSec  = 59;
        static constexpr uint16_t kStepSec = 1;
    };
    PickerLogic::Time<Range> mTime;
    std::unique_ptr<Widgets::TwoTonePicker> mPicker;
};

/// The run days: seven switches, then Done.
class RunDaysScreen : public Screen
{
public:
    explicit RunDaysScreen(Model& model);
    ~RunDaysScreen() override;
    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    void refresh();
    WheelMenu::Item mItems[8] {};
    char mTitleText[20] = "RUN DAYS";
    std::unique_ptr<WheelMenu>        mMenu;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

/// The race date: None, then each Saturday and Sunday from 3 to 26 weeks ahead.
class RaceDateScreen : public Screen
{
public:
    static constexpr uint16_t kFirstWeek = 3;
    static constexpr uint16_t kLastWeek  = 26;
    static constexpr uint16_t kMaxItems  = 1 + 2 * (kLastWeek - kFirstWeek + 1);

    explicit RaceDateScreen(Model& model);
    ~RaceDateScreen() override;
    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    uint16_t mCount = 0;
    uint32_t mDate[kMaxItems] = {};
    char     mText[kMaxItems][16] = {};
    std::unique_ptr<WheelMenu::Item[]> mItems;
    std::unique_ptr<WheelMenu>        mMenu;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

/// The new plan in a few lines and its calendar; R1 starts it.
class PlanConfirmScreen : public Screen
{
public:
    explicit PlanConfirmScreen(Model& model);
    ~PlanConfirmScreen() override;
    void onShow() override;
    void onKey(uint8_t code) override;
    void onPlanPreview() override;

protected:
    void build() override;

private:
    void fill();
    TextStack mStack;
    char      mText[TextStack::kMaxLines][48] = {};
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

/// The phone's new plan: start it, or keep yours.
class PlanAskScreen : public Screen
{
public:
    explicit PlanAskScreen(Model& model);
    ~PlanAskScreen() override;
    void onShow() override;
    void onKey(uint8_t code) override;
    void onPlanSetup() override;

protected:
    void build() override;

private:
    enum Id : uint16_t { ID_START = 0, ID_KEEP, ID_COUNT };
    WheelMenu::Item mItems[ID_COUNT] {};
    char mTip[ID_COUNT][40] = {};
    std::unique_ptr<WheelMenu>        mMenu;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

namespace NewPlanText
{
/// "10K", "Half marathon", "VO2max block"; "" for none.
const char* planName(uint8_t plan);
/// "Tue Wed Fri Sun".
void days(uint8_t mask, char* buf, size_t cap);
} // namespace NewPlanText

#endif // NEW_PLAN_SCREENS_HPP
