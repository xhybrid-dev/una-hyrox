/**
 ******************************************************************************
 * @file    Strings.hpp
 * @brief   User-visible strings shared by more than one screen.
 *
 * The TouchGFX Run app keeps these in texts.xml; here they are plain UTF-8
 * literals, which LVGL renders directly (every generated font covers ASCII).
 ******************************************************************************
 */

#ifndef STRINGS_HPP
#define STRINGS_HPP

namespace Strings
{
// Mixed case: the SDK title is 120 px wide, and "HYBRIDX RACE" in the italic
// face is wider than that, so both ends were clipped on every menu screen.
inline constexpr const char* kAppTitle  = "HybridX Race";
inline constexpr const char* kKm        = "km";
inline constexpr const char* kMi        = "mi";
inline constexpr const char* kNoValue   = "---";
} // namespace Strings

#endif // STRINGS_HPP
