/**
 ******************************************************************************
 * @file    Widgets.hpp
 * @brief   The SDK's activity-app widgets under the Widgets namespace, with
 *          the app's font filled in (as HybridX Race's).
 ******************************************************************************
 */

#ifndef WIDGETS_HPP
#define WIDGETS_HPP

#include "lvgl.h"

#include "SDK/GUI/LVGL/Battery.hpp"
#include "SDK/GUI/LVGL/Buttons.hpp"
#include "SDK/GUI/LVGL/TimerRing.hpp"
#include "SDK/GUI/LVGL/Title.hpp"
#include "gui/theme/Theme.hpp"

namespace Widgets
{

/// Screen centre, where every bezel arc is centred.
using SDK::LVGL::kCx;
using SDK::LVGL::kCy;

using Buttons   = SDK::LVGL::Buttons;
using Battery   = SDK::LVGL::Battery;
using TimerRing = SDK::LVGL::TimerRing;

/// The SDK title in Italic 18.
class Title : public SDK::LVGL::Title
{
public:
    Title(lv_obj_t* parent, const char* text)
        : SDK::LVGL::Title(parent, Theme::font(Theme::Font::Italic18), text)
    {
    }
};

} // namespace Widgets

#endif // WIDGETS_HPP
