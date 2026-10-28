/**
 ******************************************************************************
 * @file    WheelMenu.hpp
 * @brief   The SDK's scroll-wheel menu in HybridX Workout's faces (SemiBold 30
 *          selected, Medium 18 around it, Italic 18 hints) and a 400 ms slide.
 ******************************************************************************
 */

#ifndef WHEEL_MENU_HPP
#define WHEEL_MENU_HPP

#include <cstdint>

#include "lvgl.h"

#include "SDK/GUI/LVGL/WheelMenu.hpp"

class WheelMenu : public SDK::LVGL::WheelMenu
{
public:
    /**
     * @param items        Menu entries, kept alive (and possibly edited) by the caller.
     * @param count        Number of entries.
     * @param itemOffsetY  Vertical nudge of a Simple surrounding item's text.
     */
    WheelMenu(lv_obj_t* parent, const Item* items, uint16_t count, int16_t itemOffsetY = 0);
};

#endif // WHEEL_MENU_HPP
