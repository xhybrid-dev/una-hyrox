/**
 ******************************************************************************
 * @file    RouteListScreen.cpp
 * @brief   HybridX Trail: choose a route (see RouteListScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/RouteListScreen.hpp"

#include <cstdio>

#include "gui/RouteFormat.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;

namespace
{
/// The selected item's faces, largest first, and the room it has in the lens.
const lv_font_t* const kNameFaces[] = {
    Theme::font(Theme::Font::SemiBold30),
    Theme::font(Theme::Font::SemiBold25),
    Theme::font(Theme::Font::SemiBold20),
    Theme::font(Theme::Font::Medium18),   // the face of the items around it
};
constexpr int32_t kNameMaxW = 204;
} // namespace

RouteListScreen::RouteListScreen(Model& model)
    : Screen(model)
{
}

void RouteListScreen::fill()
{
    using Style      = WheelMenu::Item::Style;
    const bool imp   = mModel.isUnitsImperial();
    mItemCount       = 0;

    WheelMenu::Item& none = mItems[mItemCount++];
    none                  = WheelMenu::Item {};
    none.style            = Style::Tip;
    none.text             = "No route";
    none.tip              = "a plain run";
    none.tipColor         = Color::GRAY;

    for (uint8_t i = 0; i < mModel.routeCount() && mItemCount < kMaxItems - 1; ++i) {
        const Trail::RouteInfo& r = mModel.routeAt(i);
        RouteFmt::name(mNames[mItemCount], sizeof(mNames[0]), r.name);
        const lv_font_t* face = RouteFmt::fit(mNames[mItemCount], sizeof(mNames[0]), kNameFaces,
                                              sizeof(kNameFaces) / sizeof(kNameFaces[0]), kNameMaxW);
        if (r.points == 0) {
            std::snprintf(mTips[mItemCount], sizeof(mTips[0]), "can't read this file");
        } else {
            RouteFmt::summary(mTips[mItemCount], sizeof(mTips[0]), r.lengthM, r.ascentM, imp);
        }
        WheelMenu::Item& item = mItems[mItemCount];
        item                  = WheelMenu::Item {};
        item.style            = Style::Tip;
        item.text             = mNames[mItemCount];
        item.centerFont       = face;
        item.tip              = mTips[mItemCount];
        item.tipColor         = r.points == 0 ? Color::RED : Color::GRAY;
        ++mItemCount;
    }

    WheelMenu::Item& add = mItems[mItemCount++];
    add                  = WheelMenu::Item {};
    add.style            = Style::Tip;
    add.text             = "Add routes";
    add.tip              = "copy GPX by USB";
    add.tipColor         = Color::GRAY;
}

void RouteListScreen::build()
{
    fill();
    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, mItemCount);
    mMenu->setSlideMidCallback([](void* ctx, uint16_t) { static_cast<RouteListScreen*>(ctx)->updateButtons(); },
                               this);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, "ROUTE");
}

void RouteListScreen::onShow()
{
    // Start on the route just previewed (back from its preview), else the
    // route in use. The list starts with "No route", so route i is item i + 1.
    const int8_t sel = mModel.previewRoute() >= 0 ? mModel.previewRoute() : mModel.selectedRoute();
    mMenu->select(sel >= 0 ? static_cast<uint16_t>(sel + 1) : 0);
    updateButtons();
}

void RouteListScreen::onRoutes()
{
    // The service re-sends the list after every choice. The same routes: new
    // text in the same items. A rescan that changed their number: build the
    // screen afresh (a WheelMenu leaves its objects behind when destroyed).
    const uint16_t count = mItemCount;
    fill();
    if (mItemCount != count) {
        ScreenManager::instance().goTo(ScreenId::RouteList);
        return;
    }
    mMenu->refresh();
    updateButtons();
}

void RouteListScreen::updateButtons()
{
    const uint16_t sel     = mMenu->selected();
    const bool     canPick = sel == 0 || (isRoute(sel) && mModel.routeAt(static_cast<uint8_t>(sel - 1)).points > 0);
    mMenu->setBackground(canPick ? Color::TEAL_DARK : Color::GRAY_DARK);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  canPick ? Widgets::Buttons::AMBER : Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
}

void RouteListScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: {
            const uint16_t sel = mMenu->selected();
            if (sel == 0) {
                mModel.selectRoute(-1);
                ScreenManager::instance().goTo(ScreenId::Main);
            } else if (isRoute(sel) && mModel.routeAt(static_cast<uint8_t>(sel - 1)).points > 0) {
                mModel.setPreviewRoute(static_cast<int8_t>(sel - 1));
                ScreenManager::instance().goTo(ScreenId::RoutePreview);
            }
        } break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::Main); break;
        default: break;
    }
}
