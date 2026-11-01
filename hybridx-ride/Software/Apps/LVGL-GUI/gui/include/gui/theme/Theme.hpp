/**
 ******************************************************************************
 * @file    Theme.hpp
 * @brief   HybridX Ride's fonts and zone colours, plus the SDK's LVGL
 *          drawing helpers under the app's name (as HybridX Race's Theme).
 *
 * Colours come from the SDK's 64-colour palette (SDK/GUI/Color.hpp): the
 * display keeps two bits per channel, so those values render exactly.
 ******************************************************************************
 */

#ifndef THEME_HPP
#define THEME_HPP

#include <cstdint>
#include <cstring>

#include "lvgl.h"

#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "RideModel.hpp"
#include "gui/Audit.hpp"

namespace Theme
{

/// The Poppins faces the app uses, by weight and pixel size.
enum class Font : uint8_t {
    Italic18,
    Light60,
    Medium18,
    Medium25,
    Regular14,
    Regular18,
    SemiBold20,
    SemiBold25,
    SemiBold30,
    SemiBold40,
};

const lv_font_t* font(Font f);

/// The colour of a heart-rate zone, as the SDK's Workout example draws them
/// (SummaryFaceZones.cpp: cyan, green, yellow, orange, red, all on the
/// display's grid); grey below zone 1 or with no heart rate.
inline uint32_t zoneColor(uint8_t zone)
{
    static constexpr uint32_t kZone[5] = {0x00C0C0, 0x40C000, 0xC0C000, 0xC08000, 0xC00000};
    return zone >= 1 && zone <= 5 ? kZone[zone - 1] : SDK::GUI::Color::GRAY;
}

// The SDK's drawing helpers, under the app's name for them.
using SDK::LVGL::Draw::rgb;
using SDK::LVGL::Draw::init;
using SDK::LVGL::Draw::applyScreen;
using SDK::LVGL::Draw::setHidden;
using SDK::LVGL::Draw::container;
using SDK::LVGL::Draw::box;
using SDK::LVGL::Draw::dot;
using SDK::LVGL::Draw::arc;
using SDK::LVGL::Draw::setArc;
using SDK::LVGL::Draw::setArcColor;

/// Draw::label() with one of the app's faces. The label is also tracked by the
/// simulator's text-fit audit (Audit.hpp).
inline lv_obj_t* label(lv_obj_t* parent, Font f, const char* text,
                       int32_t x, int32_t y, int32_t w,
                       lv_text_align_t align = LV_TEXT_ALIGN_CENTER,
                       uint32_t color = SDK::GUI::Color::WHITE)
{
    lv_obj_t* l = SDK::LVGL::Draw::label(parent, font(f), text, x, y, w, align, color);
    Audit::track(l);
    return l;
}

/// Change a label's text only if it differs: the once-a-second redraws then
/// leave an unchanged label alone (no reallocation, no repaint).
inline void assign(lv_obj_t* l, const char* text)
{
    const char* now = lv_label_get_text(l);
    if (now == nullptr || std::strcmp(now, text) != 0) {
        lv_label_set_text(l, text);
    }
}

/// Change a label's text and colour together.
inline void setText(lv_obj_t* l, const char* text, uint32_t color)
{
    lv_label_set_text(l, text);
    lv_obj_set_style_text_color(l, rgb(color), LV_PART_MAIN);
}

} // namespace Theme

#endif // THEME_HPP
