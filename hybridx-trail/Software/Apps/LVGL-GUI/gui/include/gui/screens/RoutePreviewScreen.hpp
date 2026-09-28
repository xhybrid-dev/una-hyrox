/**
 ******************************************************************************
 * @file    RoutePreviewScreen.hpp
 * @brief   HybridX Trail: the whole route before choosing it.
 *
 * Opening the preview loads the route in the service (so the map is the route
 * the watch will follow, thinned exactly as it will be used), and shows it
 * whole, north-up: amber line, lime start, red finish, with its distance and
 * climb. R1 keeps it and goes back to the start screen; R2 puts back the
 * route chosen before and returns to the list.
 ******************************************************************************
 */

#ifndef ROUTE_PREVIEW_SCREEN_HPP
#define ROUTE_PREVIEW_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/RouteMap.hpp"
#include "gui/widgets/Widgets.hpp"

class RoutePreviewScreen : public Screen
{
public:
    explicit RoutePreviewScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onRoute() override;

protected:
    void build() override;

private:
    void show();

    int8_t mPrevious = -1;   ///< the route in use before the preview
    int8_t mIndex    = -1;   ///< the route being previewed
    char   mName[32] {};
    char   mLine[40] {};

    std::unique_ptr<Widgets::Title>    mTitle;
    std::unique_ptr<Widgets::RouteMap> mMap;
    std::unique_ptr<Widgets::Buttons>  mButtons;
    lv_obj_t*                          mNameLbl = nullptr;
    lv_obj_t*                          mSummary = nullptr;
    lv_obj_t*                          mLoading = nullptr;
};

#endif // ROUTE_PREVIEW_SCREEN_HPP
