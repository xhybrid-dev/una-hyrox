/**
 ******************************************************************************
 * @file    GuiApp.cpp
 * @brief   The bike probe's GUI entry point (as Knitting's).
 ******************************************************************************
 */

#include <new>

#include "gui/Assets.hpp"
#include "gui/model/Model.hpp"
#include "gui/screens/MainScreen.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace
{
alignas(Model)      uint8_t sModelStorage[sizeof(Model)];
alignas(MainScreen) uint8_t sScreenStorage[sizeof(MainScreen)];
} // namespace

extern "C" const lv_font_t* una_lvgl_default_font(void)
{
    return &poppins_regular_18;
}

extern "C" void una_lvgl_app_init(void)
{
    SDK::LVGL::Draw::init();
    Model*      model  = new (sModelStorage) Model();
    MainScreen* screen = new (sScreenStorage) MainScreen(*model);
    lv_screen_load(screen->root());
}
