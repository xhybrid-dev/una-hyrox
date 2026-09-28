/**
 ******************************************************************************
 * @file    RoutePreviewScreen.cpp
 * @brief   HybridX Trail: the whole route (see RoutePreviewScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/RoutePreviewScreen.hpp"


#include "gui/RouteFormat.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;
using F = Theme::Font;

namespace
{
constexpr int32_t kNameMaxW = 116;   ///< the SDK title label is 120 px wide
} // namespace

RoutePreviewScreen::RoutePreviewScreen(Model& model)
    : Screen(model)
{
}

void RoutePreviewScreen::build()
{
    // The promo's "Pick a route": the name, the route glowing, its distance
    // and climb, and what R1 does.
    mMap     = std::make_unique<Widgets::RouteMap>(mRoot, 30, 44, 180, 124);
    mSummary = Theme::label(mRoot, F::SemiBold20, "", 10, 170, 220);
    mHint    = Theme::label(mRoot, F::Regular16, "R1 to start", 40, 202, 160, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mLoading = Theme::label(mRoot, F::Italic18, "Reading route...", 20, 100, 200, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "");
}

void RoutePreviewScreen::onShow()
{
    mPrevious = mModel.selectedRoute();
    mIndex    = mModel.previewRoute();
    if (mIndex >= 0 && mIndex < static_cast<int8_t>(mModel.routeCount())) {
        // The title, cut to the room it has.
        static const lv_font_t* const kFaces[] = { Theme::font(F::Italic18) };
        RouteFmt::name(mName, sizeof(mName), mModel.routeAt(static_cast<uint8_t>(mIndex)).name);
        RouteFmt::fit(mName, sizeof(mName), kFaces, 1, kNameMaxW);
        mTitle->setText(mName);
    }
    if (mIndex == mPrevious && mModel.hasRoute()) {
        show();   // already loaded
    } else {
        Theme::setHidden(mMap->root(), true);
        mModel.selectRoute(mIndex);   // the service answers with onRoute()
    }
}

void RoutePreviewScreen::onRoute()
{
    show();
}

void RoutePreviewScreen::show()
{
    if (!mModel.hasRoute()) {
        lv_label_set_text(mLoading, "Can't read this route");
        return;
    }
    Theme::setHidden(mLoading, true);
    Theme::setHidden(mMap->root(), false);
    mMap->setRoute(mModel.routePoints(), mModel.routePointCount());
    mMap->fitWhole(nullptr, 14);
    const Trail::RouteInfo& r = mModel.route();
    RouteFmt::summary(mLine, sizeof(mLine), r.lengthM, r.ascentM, mModel.isUnitsImperial());
    lv_label_set_text(mSummary, mLine);
}

void RoutePreviewScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::R1:
            if (mModel.hasRoute()) {
                // Start with it, as the start screen does: straight away with a
                // GPS fix, else the "no GPS, start anyway?" question.
                if (mModel.hasGpsFix()) {
                    mModel.trackStart(false);
                    ScreenManager::instance().goTo(ScreenId::Track);
                } else {
                    mModel.setPendingIntervalsMode(false);
                    ScreenManager::instance().goTo(ScreenId::TrackStartConfirm);
                }
            }
            break;
        case Btn::R2:
            if (mPrevious != mIndex) {
                mModel.selectRoute(mPrevious);   // put back what was chosen before
            }
            ScreenManager::instance().goTo(ScreenId::RouteList);
            break;
        default:
            break;
    }
}
