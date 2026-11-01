/**
 ******************************************************************************
 * @file    Audit.hpp
 * @brief   The simulator's text-fit audit: does every label of ours, with the
 *          worst text it can be given, sit inside its box and clear of the
 *          bezel arcs? Compiled to nothing on the watch.
 *
 * The face is a 240 px circle and the buttons' arcs run along its edge with
 * their inner edge at a radius of 110 (SDK Buttons: radius 113, 6 px wide), so
 * a label whose text is entirely within kArcClear of the centre cannot touch
 * one (the rule Trail Quest's Layout.hpp states). The labels are LONG_CLIP:
 * text wider than its box is cut off without a word, which this also catches.
 *
 * Poppins is proportional, so the width is measured with LVGL on the real font,
 * not counted in characters. The labels the SDK's own widgets draw (the wheel
 * menu, the title) are the SDK's design and are not tracked.
 ******************************************************************************
 */

#ifndef AUDIT_HPP
#define AUDIT_HPP

#include "lvgl.h"

namespace Audit
{

/// Distance from the face's centre that text may reach: inside the arcs' inner edge.
constexpr int32_t kArcClear = 109;

#if defined(SIMULATOR)
/// Remember a label to be checked. Forgotten when the label is deleted.
void track(lv_obj_t* label);

/// Check every tracked, visible label with its current text. Logs each problem
/// and returns how many there were.
int check(const char* where);
#else
inline void track(lv_obj_t*) {}
inline int check(const char*) { return 0; }
#endif

} // namespace Audit

#endif // AUDIT_HPP
