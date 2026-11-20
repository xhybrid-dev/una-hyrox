/**
 ******************************************************************************
 * @file    FeelScreen.hpp
 * @brief   X Training (TP8): how the runner felt, asked once after a planned
 *          session is saved (Jon, 20 November 2026: after only). Fresh, Good,
 *          Steady, Tired, Spent, or Skip. L1/L2 move, R1 chooses, R2 skips.
 *          The answer goes to the service (PlanAction::Feel), which keeps it
 *          beside the session's review (reviews.json); then the flow goes on
 *          to the treadmill's distance or the summary.
 ******************************************************************************
 */

#ifndef FEEL_SCREEN_HPP
#define FEEL_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/WheelMenu.hpp"
#include "gui/widgets/Widgets.hpp"

class FeelScreen : public Screen
{
public:
    explicit FeelScreen(Model& model);
    ~FeelScreen() override;
    void onShow() override;
    void onKey(uint8_t code) override;

protected:
    void build() override;

private:
    /// The five feelings in the order of Review::Feel (1 to 5), then Skip.
    enum Id : uint16_t { ID_FRESH = 0, ID_GOOD, ID_STEADY, ID_TIRED, ID_SPENT, ID_SKIP, ID_COUNT };
    void answer(uint8_t feel);   ///< 0 = skipped

    WheelMenu::Item mItems[ID_COUNT] {};
    std::unique_ptr<WheelMenu>        mMenu;
    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // FEEL_SCREEN_HPP
