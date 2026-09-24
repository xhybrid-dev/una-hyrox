/**
 ******************************************************************************
 * @file    GuiApp.cpp
 * @brief   HybridX Streak's GUI entry: builds the model and shows the first screen.
 *
 * una_lvgl_app_init() is the hook the SDK's LVGL entry point calls once LVGL
 * and the display are up. Objects are constructed here, not as globals, so
 * nothing touches the kernel before main() has bound it.
 ******************************************************************************
 */

#include <new>

#include "gui/Assets.hpp"
#include "gui/model/Model.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"

namespace
{
alignas(Model) uint8_t sModelStorage[sizeof(Model)];
Model* sModel = nullptr;
} // namespace

/// LVGL's default font: one of the app's own faces keeps Montserrat out of
/// the link. Every label sets its font, so this only backs stray widgets.
extern "C" const lv_font_t* una_lvgl_default_font(void)
{
    return &poppins_regular_16;
}

extern "C" void una_lvgl_app_init(void)
{
    Theme::init();
    sModel = new (sModelStorage) Model();
    ScreenManager::instance().start(*sModel, sModel->entryScreen());
}
