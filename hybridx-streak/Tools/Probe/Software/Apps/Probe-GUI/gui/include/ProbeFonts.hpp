/**
 ******************************************************************************
 * @file    ProbeFonts.hpp
 * @brief   The three Poppins faces the probe borrows from HybridX Streak.
 *
 * Also the probe GUI's include directory: the SDK's simulator sources find
 * ConfigurationSimulator.hpp at "<include dir>/../../simulator", as they do
 * for the Streak app's gui/include.
 ******************************************************************************
 */

#ifndef STREAK_PROBE_FONTS_HPP
#define STREAK_PROBE_FONTS_HPP

#include "lvgl.h"

extern "C" {
extern const lv_font_t poppins_semibold_25;
extern const lv_font_t poppins_regular_16;
extern const lv_font_t poppins_regular_14;
}

#endif // STREAK_PROBE_FONTS_HPP
