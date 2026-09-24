/**
 ******************************************************************************
 * @file    Assets.hpp
 * @brief   Declarations of the generated fonts and images under assets/.
 *
 * The font and image C files were generated for HybridX Race by its
 * assets/gen_assets.py (lv_font_conv 1.5.3, Poppins at 2 bpp) and copied here
 * unchanged: only the faces this app uses.
 ******************************************************************************
 */

#ifndef STREAK_ASSETS_HPP
#define STREAK_ASSETS_HPP

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Printable ASCII plus U+00B7 (the middle dot) in every face except SemiBold 60,
// which carries digits and punctuation only (plus A/M/O/P/e/n/p).
LV_FONT_DECLARE(poppins_italic_18);
LV_FONT_DECLARE(poppins_medium_18);
LV_FONT_DECLARE(poppins_regular_14);
LV_FONT_DECLARE(poppins_regular_16);
LV_FONT_DECLARE(poppins_semibold_20);
LV_FONT_DECLARE(poppins_semibold_25);
LV_FONT_DECLARE(poppins_semibold_30);
LV_FONT_DECLARE(poppins_semibold_60);

// A8 alpha masks, tinted at draw time: the UNA convention of a tick beside R1
// and a cross beside R2 for confirm / decline.
LV_IMAGE_DECLARE(img_tickgreen_22x17);
LV_IMAGE_DECLARE(img_crosswhite_17x17);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // STREAK_ASSETS_HPP
