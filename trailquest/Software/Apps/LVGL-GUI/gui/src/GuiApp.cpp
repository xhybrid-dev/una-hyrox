/**
 ******************************************************************************
 * @file    GuiApp.cpp
 * @brief   Trail Quest GUI entry: builds the model and shows the Q0 probe.
 *
 * una_lvgl_app_init() is the hook the SDK's LVGL entry point calls once LVGL
 * and the display are up. It owns the model and the screen for the life of
 * the process.
 ******************************************************************************
 */

#include <new>

#include "gui/Assets.hpp"
#include "gui/model/Model.hpp"
#include "gui/screens/ProbeScreen.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace
{
alignas(Model) uint8_t sModelStorage[sizeof(Model)];
alignas(ProbeScreen) uint8_t sScreenStorage[sizeof(ProbeScreen)];
} // namespace

/// LVGL's default font: one of the app's own, so the built-in font stays out
/// of the link.
extern "C" const lv_font_t* una_lvgl_default_font(void)
{
    return &poppins_regular_18;
}

extern "C" void una_lvgl_app_init(void)
{
    SDK::LVGL::Draw::init();
    Model* model = new (sModelStorage) Model();
    ProbeScreen* screen = new (sScreenStorage) ProbeScreen(*model);
    screen->show();
}
