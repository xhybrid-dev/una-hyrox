/**
 ******************************************************************************
 * @file    Vo2Screen.hpp
 * @brief   HybridX Run: the VO2max page (R5), from the start screen's wheel.
 *
 * Page 1, the overview: the figure shown (the latest runs, weighted), its
 * trend since the last run, and the max and resting HR in use with where they
 * come from (or what the profile still needs). Then the last runs, newest
 * first, five to a page. L1/L2 page; R1 opens the profile menu; R2 goes back
 * to the start screen. Only the page on display exists (the summary's
 * pattern, NOTES R3), and it is rebuilt when a fresh Vo2Info arrives.
 ******************************************************************************
 */

#ifndef VO2_SCREEN_HPP
#define VO2_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class Vo2Screen : public Screen
{
public:
    explicit Vo2Screen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onVo2Info(const CustomMessage::Vo2Info& info) override;
    /// Idle on a menu screen leaves the app, as RunLVGL's menus do.
    void onIdleTimeout() override { mModel.exitApp(); }

protected:
    void build() override;

private:
    static constexpr uint8_t kRowsPerPage = 5;

    uint8_t pageCount() const;
    void    showPage(uint8_t page);
    void    buildOverview(lv_obj_t* f);
    void    buildRuns(lv_obj_t* f, uint8_t listPage);

    lv_obj_t* mPage   = nullptr;   ///< the one page built
    uint8_t   mPageId = 0;         ///< 0 = overview, 1.. = run pages

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<Widgets::ScrollIndicator> mIndicator;
};

#endif // VO2_SCREEN_HPP
