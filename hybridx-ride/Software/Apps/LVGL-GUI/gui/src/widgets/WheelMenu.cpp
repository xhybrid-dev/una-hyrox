/**
 ******************************************************************************
 * @file    WheelMenu.cpp
 * @brief   The SDK's scroll-wheel menu in HybridX Ride's faces and timing.
 ******************************************************************************
 */

#include "gui/widgets/WheelMenu.hpp"

#include "gui/theme/Theme.hpp"

namespace
{
constexpr uint32_t kMenuAnimationMs = 400;

SDK::LVGL::WheelMenu::Fonts menuFonts()
{
    return { Theme::font(Theme::Font::SemiBold30),
             Theme::font(Theme::Font::Medium18),
             Theme::font(Theme::Font::Italic18) };
}
} // namespace

WheelMenu::WheelMenu(lv_obj_t* parent, const Item* items, uint16_t count, int16_t itemOffsetY)
    : SDK::LVGL::WheelMenu(parent, items, count, menuFonts(), itemOffsetY)
{
    setAnimationMs(kMenuAnimationMs);
}
