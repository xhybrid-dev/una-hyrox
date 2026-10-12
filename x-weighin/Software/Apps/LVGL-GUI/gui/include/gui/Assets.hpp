/**
 ******************************************************************************
 * @file    Assets.hpp
 * @brief   Declarations of the generated fonts under assets/.
 *
 * The definitions were generated for the SDK's Waypoint tutorial (from its
 * assets/assets.json, by Utilities/Scripts/lvgl_assets/lvgl_assets.py) and
 * copied unchanged: Poppins, printable ASCII (0x20-0x7E), 2 bpp.
 ******************************************************************************
 */

#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "lvgl.h"

// The generated definitions are C, so the names must not be mangled. GCC
// leaves global variables unmangled anyway; MSVC (the PC simulator) does not.
#ifdef __cplusplus
extern "C" {
#endif

LV_FONT_DECLARE(poppins_semibold_20);   // title
LV_FONT_DECLARE(poppins_semibold_25);   // kept from Waypoint for W2's screens
LV_FONT_DECLARE(poppins_semibold_30);   // kept from Waypoint for W2's screens
LV_FONT_DECLARE(poppins_semibold_40);   // kept from Waypoint for W2's screens
LV_FONT_DECLARE(poppins_regular_18);    // rows and status line

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ASSETS_HPP
