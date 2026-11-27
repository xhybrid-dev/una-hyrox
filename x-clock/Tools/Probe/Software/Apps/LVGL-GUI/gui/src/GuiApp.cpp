/**
 ******************************************************************************
 * @file    GuiApp.cpp
 * @brief   The Clock Probe's GUI entry point (as the Energy Probe's).
 ******************************************************************************
 */

#include <new>

#include "gui/Assets.hpp"
#include "gui/model/Model.hpp"
#include "gui/screens/ProbeScreen.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace
{
alignas(Model)       uint8_t sModelStorage[sizeof(Model)];
alignas(ProbeScreen) uint8_t sScreenStorage[sizeof(ProbeScreen)];
} // namespace

extern "C" const lv_font_t* una_lvgl_default_font(void)
{
    return &poppins_regular_14;
}

extern "C" void una_lvgl_app_init(void)
{
    SDK::LVGL::Draw::init();
    Model*       model  = new (sModelStorage) Model();
    ProbeScreen* screen = new (sScreenStorage) ProbeScreen(*model);
    lv_screen_load(screen->root());
    // One firing per kernel tick (lv_timer_handler runs once a frame): the probe's frame counter.
    lv_timer_create(&Model::frameTimerCb, 1, model);
}
