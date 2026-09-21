/**
 ******************************************************************************
 * @file    TrackStartConfirmScreen.cpp
 * @brief   "Start before signal acquired?" confirmation.
 ******************************************************************************
 */

#include "gui/screens/TrackStartConfirmScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Strings.hpp"

#include "RaceData.hpp"
#include "RaceModel.hpp"

#include <cstdio>

TrackStartConfirmScreen::TrackStartConfirmScreen(Model& model)
    : Screen(model)
{
}

void TrackStartConfirmScreen::build()
{
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);

    // The screen inherited from RunLVGL asked "Start before signal acquired?".
    // HYROX is indoors and brief 8.2 rules out GPS gating entirely, so there is
    // no signal to wait for: what this screen guards is an accidental press, and
    // what the athlete needs to see is the race they are about to start.
    Theme::label(mRoot, Theme::Font::SemiBold25, "On your marks", 0, 86, 240);
    // Three short lines rather than one long one: at y = 150 the display is
    // about 225 px across, and "16 segments \xC2\xB7 Roxzone merged" all but
    // touches the bezel at both ends (NOTES.md 4.3).
    mFormat   = Theme::label(mRoot, Theme::Font::Medium18, "", 0, 116, 240,
                             LV_TEXT_ALIGN_CENTER, SDK::GUI::Color::CYAN);
    mSegments = Theme::label(mRoot, Theme::Font::Regular14, "", 0, 140, 240,
                             LV_TEXT_ALIGN_CENTER, SDK::GUI::Color::GRAY);
    mRoxzone  = Theme::label(mRoot, Theme::Font::Regular14, "", 0, 158, 240,
                             LV_TEXT_ALIGN_CENTER, SDK::GUI::Color::GRAY);

    Theme::imageTinted(mRoot, &img_tickgreen_22x17, 186, 60, SDK::GUI::Color::YELLOW_DARK);
    mTitle = std::make_unique<Widgets::Title>(mRoot, Strings::kAppNameUc);
}

void TrackStartConfirmScreen::onShow()
{
    // Deliberately no resetIdleTimer(), and no onIdleTimeout(): this is where an
    // athlete stands waiting for the gun, and brief 8.4's menu rule must not
    // throw them out of the app thirty seconds before their race.
    const Settings &s = mModel.getSettings();

    char buf[32];
    const char *format = "Full race";
    switch (s.format) {
    case Race::Format::HalfA: format = "Half: rounds 1-4"; break;
    case Race::Format::HalfB: format = "Half: rounds 5-8"; break;
    case Race::Format::Full:
    default:                  break;
    }
    snprintf(buf, sizeof(buf), "%s", format);
    lv_label_set_text(mFormat, buf);

    snprintf(buf, sizeof(buf), "%u segments",
             static_cast<unsigned>(
                     Race::RaceModel::plannedCount(s.format, s.roxzoneSplits)));
    lv_label_set_text(mSegments, buf);

    lv_label_set_text(mRoxzone, s.roxzoneSplits ? "Roxzone split" : "Roxzone merged");
}

void TrackStartConfirmScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code == Btn::R1) {
        // US1: the clock must be running within one GUI tick of this press, so
        // start first and navigate second.
        mModel.raceStart();
        ScreenManager::instance().goTo(ScreenId::Race);
    } else if (code == Btn::R2) {
        ScreenManager::instance().goTo(ScreenId::Main);
    }
}
