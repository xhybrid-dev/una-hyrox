/**
 ******************************************************************************
 * @file    RouteListScreen.hpp
 * @brief   HybridX Trail: choose a route (or none) from Routes/.
 *
 * A wheel of the routes the service found, by name, each with its distance and
 * climb; "No route" first, for a plain run; and, last, how to add routes. R1
 * on a route opens its preview; on "No route" it clears the route and goes
 * back. R2 goes back.
 ******************************************************************************
 */

#ifndef ROUTE_LIST_SCREEN_HPP
#define ROUTE_LIST_SCREEN_HPP

#include <memory>

#include "Navigator.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/WheelMenu.hpp"
#include "gui/widgets/Widgets.hpp"

class RouteListScreen : public Screen
{
public:
    explicit RouteListScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    void onRoutes() override;

protected:
    void build() override;

private:
    static constexpr uint16_t kMaxItems = Trail::Navigator::kMaxRoutes + 2;   ///< + "No route" + "Add routes"

    void fill();
    void updateButtons();
    bool isRoute(uint16_t item) const { return item >= 1 && item <= mModel.routeCount(); }

    WheelMenu::Item mItems[kMaxItems] {};
    char            mNames[kMaxItems][32] {};
    char            mTips[kMaxItems][32] {};
    uint16_t        mItemCount = 0;

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // ROUTE_LIST_SCREEN_HPP
