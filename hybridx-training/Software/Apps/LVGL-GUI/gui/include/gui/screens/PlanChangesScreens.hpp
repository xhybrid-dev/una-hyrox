/**
 ******************************************************************************
 * @file    PlanChangesScreens.hpp
 * @brief   X Training (TP5): the plan's changes in the moment (brief 5.2), and the answers to
 *          its offers and a trial's question.
 *
 * Changes (the main menu): do tomorrow's session today, move today's to tomorrow, skip today,
 * pause (or resume) the plan, end it (R1 twice). Each is PlanRules' (PlanRules.hpp); the
 * service answers with Today, whose title carries the result (Moved, Not allowed, No session).
 *
 * PlanOfferScreen answers what Today asks: an offer (a missed session to do now: Yes or No), or
 * the 5K a time trial gave (update your paces, or keep them).
 ******************************************************************************
 */

#ifndef PLAN_CHANGES_SCREENS_HPP
#define PLAN_CHANGES_SCREENS_HPP

#include <memory>

#include "gui/model/AppMenu.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/WheelMenu.hpp"
#include "gui/widgets/Widgets.hpp"

/// Move, skip, pause or end the plan.
class PlanChangesScreen : public Screen
{
public:
    explicit PlanChangesScreen(Model& model);
    ~PlanChangesScreen() override;
    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    using Menu = App::MenuNav::Root::Changes;
    void refresh();
    bool planUnderWay() const;

    WheelMenu::Item mItems[Menu::ID_COUNT] {};
    bool mConfirmEnd = false;   ///< R1 on End plan once: it asks again
    std::unique_ptr<WheelMenu>        mMenu;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

/// An offer, or a trial's question: two choices.
class PlanOfferScreen : public Screen
{
public:
    explicit PlanOfferScreen(Model& model);
    ~PlanOfferScreen() override;
    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    enum Id : uint16_t { ID_YES = 0, ID_NO, ID_COUNT };
    bool pace() const;   ///< a trial's question, not an offer

    WheelMenu::Item mItems[ID_COUNT] {};
    char mText[ID_COUNT][24] = {};
    char mTip[ID_COUNT][32] = {};
    char mTitleText[20] = {};
    std::unique_ptr<WheelMenu>        mMenu;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // PLAN_CHANGES_SCREENS_HPP
