/**
 ******************************************************************************
 * @file    SummaryScreen.hpp
 * @brief   What the ride came to, in pages: the overview, (outdoors) speed and
 *          climb, time in each heart-rate zone, heart rate and calories, then
 *          the laps three to a page.
 *
 * L1 and L2 turn the pages (round), R2 goes back to the main menu. The laps
 * are asked of the service a page at a time (SESSION_LAPS_REQUEST) because
 * the GUI keeps none of them. Exits the app on idle.
 ******************************************************************************
 */

#ifndef SUMMARY_SCREEN_HPP
#define SUMMARY_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"

class SummaryScreen : public Screen
{
public:
    explicit SummaryScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onIdleTimeout() override;
    void onLapsPage(const Ride::LapsPageData& page) override;

protected:
    void build() override;

private:
    static constexpr uint8_t kLapsPerScreen = Ride::kLapsPerPage;
    /// Overview, zones and heart rate; outdoors a Ride page (speed, climb) too.
    uint8_t fixedPages() const { return mModel.saved().outdoor ? 4 : 3; }

    uint8_t pageCount() const;
    /// One page exists at a time (the LVGL pool is small): built when shown,
    /// deleted when left.
    void showPage(uint8_t page);
    void buildOverview();
    void buildRide();
    void buildZones();
    void buildHeart();
    void buildLaps();
    void fillLaps(const Ride::LapsPageData& page);

    uint8_t mPage        = 0;
    uint8_t mWantedFirst = 0;

    std::unique_ptr<Widgets::Title> mTitle;
    lv_obj_t* mPageRoot = nullptr;

    // Only the labels of the page on show are set
    lv_obj_t* mLapName[kLapsPerScreen] = {};
    lv_obj_t* mLapTime[kLapsPerScreen] = {};
    lv_obj_t* mLapAvg[kLapsPerScreen]  = {};
    lv_obj_t* mLapRec[kLapsPerScreen]  = {};

    std::unique_ptr<Widgets::Buttons> mButtons;
};

#endif // SUMMARY_SCREEN_HPP
