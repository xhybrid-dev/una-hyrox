/**
 ******************************************************************************
 * @file    AddRoutesScreen.cpp
 * @brief   HybridX Trail: where GPX files go (see AddRoutesScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/AddRoutesScreen.hpp"

#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

using namespace SDK::GUI;
using F = Theme::Font;

namespace
{
constexpr uint32_t kFolderAmber = Color::YELLOW_DARK;   ///< the folder path, in the button hints' amber (C0 80 00)
} // namespace

AddRoutesScreen::AddRoutesScreen(Model& model)
    : Screen(model)
{
}

void AddRoutesScreen::build()
{
    // Widths measured in Poppins: the App ID is 211 px in SemiBold 20, inside
    // the 236 px the circle has at its row.
    Theme::label(mRoot, F::Medium18, "Over USB, copy GPX", 0, 46, 240);
    Theme::label(mRoot, F::Medium18, "files into:", 0, 70, 240);
    Theme::label(mRoot, F::Regular16, "Apps", 0, 102, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    Theme::label(mRoot, F::SemiBold20, mModel.appFolder(), 0, 122, 240, LV_TEXT_ALIGN_CENTER, kFolderAmber);
    Theme::label(mRoot, F::SemiBold20, "Routes", 0, 150, 240, LV_TEXT_ALIGN_CENTER, kFolderAmber);
    Theme::label(mRoot, F::Regular16, "They show here when", 0, 182, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    Theme::label(mRoot, F::Regular16, "X Trail next opens", 0, 200, 240, LV_TEXT_ALIGN_CENTER, Color::GRAY);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "Add routes");
}

void AddRoutesScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code == Btn::R1 || code == Btn::R2) {
        ScreenManager::instance().goTo(ScreenId::RouteList);
    }
}
