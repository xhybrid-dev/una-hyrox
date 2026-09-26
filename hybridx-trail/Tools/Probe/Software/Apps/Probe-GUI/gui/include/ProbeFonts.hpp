/**
 ******************************************************************************
 * @file    ProbeFonts.hpp
 * @brief   The three Poppins faces the probe uses (copied from HybridX
 *          Streak's committed font set; same Poppins OFL source, standalone
 *          here so this probe has no dependency on another app's tree).
 *
 * Also the probe GUI's include directory: the SDK's simulator sources find
 * ConfigurationSimulator.hpp at "<include dir>/../../simulator".
 ******************************************************************************
 */

#ifndef TRAIL_PROBE_FONTS_HPP
#define TRAIL_PROBE_FONTS_HPP

#include "lvgl.h"

extern "C" {
extern const lv_font_t poppins_semibold_25;
extern const lv_font_t poppins_regular_16;
extern const lv_font_t poppins_regular_14;
}

#endif // TRAIL_PROBE_FONTS_HPP
