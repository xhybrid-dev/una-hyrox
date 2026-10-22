/**
 ******************************************************************************
 * @file    Theme.cpp
 * @brief   X Sauna's font table (see the header).
 ******************************************************************************
 */

#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"

namespace Theme
{

const lv_font_t* font(Font f)
{
    switch (f) {
        case Font::Italic18:   return &poppins_italic_18;
        case Font::Light60:    return &poppins_light_60;
        case Font::Medium18:   return &poppins_medium_18;
        case Font::Medium25:   return &poppins_medium_25;
        case Font::Regular14:  return &poppins_regular_14;
        case Font::Regular18:  return &poppins_regular_18;
        case Font::SemiBold20: return &poppins_semibold_20;
        case Font::SemiBold25: return &poppins_semibold_25;
        case Font::SemiBold30: return &poppins_semibold_30;
        case Font::SemiBold40: return &poppins_semibold_40;
    }
    return &poppins_regular_18;
}

} // namespace Theme
