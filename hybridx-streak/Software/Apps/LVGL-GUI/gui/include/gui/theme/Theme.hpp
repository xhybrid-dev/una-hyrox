/**
 ******************************************************************************
 * @file    Theme.hpp
 * @brief   HybridX Streak's fonts and palette, plus the SDK's drawing helpers.
 *
 * The palette is semantic: screens ask for Palette::kWin, not a colour, so the
 * teal-and-lime look (docs/DESIGN.md) is defined in exactly one place. Every
 * value is one of the SDK's 64 display colours (SDK/GUI/Color.hpp), so it
 * renders exactly: the panel keeps two bits per channel.
 ******************************************************************************
 */

#ifndef STREAK_THEME_HPP
#define STREAK_THEME_HPP

#include <cstdint>

#include "lvgl.h"

#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

namespace Palette
{
using namespace SDK::GUI::Color;

constexpr uint32_t kSky       = BLACK;
constexpr uint32_t kStar      = GRAY_DARK;
constexpr uint32_t kStarLit   = GRAY;
constexpr uint32_t kFarRange  = GRAY_DARK;    ///< distant mountains (STEEL_DARK read as purple)
constexpr uint32_t kRockShade = TEAL_DARK;    ///< the shaded face
constexpr uint32_t kRockLit   = TEAL;         ///< the sunlit face
constexpr uint32_t kSnowLit   = WHITE;
constexpr uint32_t kSnowShade = GRAY;
constexpr uint32_t kTrail     = GRAY;         ///< trail still to climb
constexpr uint32_t kWin       = LIME;         ///< climbed trail, the streak, achievements
constexpr uint32_t kText      = WHITE;
constexpr uint32_t kTextSoft  = GRAY;
constexpr uint32_t kAtRisk    = YELLOW_DARK;  ///< amber: the streak needs you
constexpr uint32_t kShield    = CYAN;
constexpr uint32_t kSun       = YELLOW_DARK;
constexpr uint32_t kSunRay    = LEMON;
} // namespace Palette

namespace Theme
{

enum class Font : uint8_t {
    Italic18,
    Medium18,
    Regular14,
    Regular16,
    SemiBold20,
    SemiBold25,
    SemiBold30,
    SemiBold60,   ///< digits only
};

const lv_font_t* font(Font f);

using SDK::LVGL::Draw::rgb;
using SDK::LVGL::Draw::init;
using SDK::LVGL::Draw::applyScreen;
using SDK::LVGL::Draw::setHidden;
using SDK::LVGL::Draw::container;
using SDK::LVGL::Draw::label;
using SDK::LVGL::Draw::box;
using SDK::LVGL::Draw::imageTinted;
using SDK::LVGL::Draw::dot;

/// Draw::label() with one of the app's faces.
inline lv_obj_t* label(lv_obj_t* parent, Font f, const char* text,
                       int32_t x, int32_t y, int32_t w,
                       lv_text_align_t align = LV_TEXT_ALIGN_CENTER,
                       uint32_t color = SDK::GUI::Color::WHITE)
{
    return SDK::LVGL::Draw::label(parent, font(f), text, x, y, w, align, color);
}

/// Recolour a label.
inline void setColor(lv_obj_t* label, uint32_t color)
{
    lv_obj_set_style_text_color(label, rgb(color), LV_PART_MAIN);
}

} // namespace Theme

#endif // STREAK_THEME_HPP
