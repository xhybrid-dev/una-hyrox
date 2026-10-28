/**
 ******************************************************************************
 * @file    Audit.cpp
 * @brief   The simulator's text-fit audit (see the header).
 ******************************************************************************
 */

#include "gui/Audit.hpp"

#if defined(SIMULATOR)

#include <cstdio>
#include <cstring>

#define LOG_MODULE_PRX      "Audit"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace Audit
{

namespace
{
constexpr int kMax = 64;
lv_obj_t* sLabels[kMax];
int       sCount = 0;

void onDelete(lv_event_t* e)
{
    lv_obj_t* o = static_cast<lv_obj_t*>(lv_event_get_target(e));
    for (int i = 0; i < sCount; ++i) {
        if (sLabels[i] == o) {
            sLabels[i] = sLabels[--sCount];
            return;
        }
    }
}

bool hidden(const lv_obj_t* o)
{
    for (; o != nullptr; o = lv_obj_get_parent(o)) {
        if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) {
            return true;
        }
    }
    return false;
}

int32_t distSq(int32_t x, int32_t y)
{
    const int32_t dx = x - 120;
    const int32_t dy = y - 120;
    return dx * dx + dy * dy;
}
} // namespace

void track(lv_obj_t* label)
{
    if (sCount < kMax) {
        sLabels[sCount++] = label;
        lv_obj_add_event_cb(label, onDelete, LV_EVENT_DELETE, nullptr);
    }
}

int check(const char* where)
{
    int problems = 0;
    for (int i = 0; i < sCount; ++i) {
        lv_obj_t* o = sLabels[i];
        if (hidden(o)) {
            continue;
        }
        const char* text = lv_label_get_text(o);
        if (text == nullptr || text[0] == '\0') {
            continue;
        }
        lv_obj_update_layout(o);
        lv_area_t a;
        lv_obj_get_coords(o, &a);
        const lv_font_t* font = lv_obj_get_style_text_font(o, LV_PART_MAIN);
        lv_point_t size;
        lv_text_get_size(&size, text, font, lv_obj_get_style_text_letter_space(o, LV_PART_MAIN),
                         lv_obj_get_style_text_line_space(o, LV_PART_MAIN), LV_COORD_MAX, LV_TEXT_FLAG_NONE);

        const int32_t boxW = a.x2 - a.x1 + 1;
        int32_t       x0   = a.x1;
        switch (lv_obj_get_style_text_align(o, LV_PART_MAIN)) {
            case LV_TEXT_ALIGN_CENTER: x0 = a.x1 + (boxW - size.x) / 2; break;
            case LV_TEXT_ALIGN_RIGHT:  x0 = a.x2 + 1 - size.x; break;
            default:                   break;
        }
        const int32_t x1 = x0 + size.x - 1;
        const int32_t y0 = a.y1;
        const int32_t y1 = a.y1 + size.y - 1;

        if (size.x > boxW) {
            LOG_ERROR("%s: '%s' is %d px wide in a %d px box (clipped)\n", where, text,
                      static_cast<int>(size.x), static_cast<int>(boxW));
            ++problems;
        }
        const int32_t cx[4] = {x0, x1, x0, x1};
        const int32_t cy[4] = {y0, y0, y1, y1};
        for (int c = 0; c < 4; ++c) {
            if (distSq(cx[c], cy[c]) > kArcClear * kArcClear) {
                LOG_ERROR("%s: '%s' reaches (%d,%d), outside the arcs' clear radius %d\n", where, text,
                          static_cast<int>(cx[c]), static_cast<int>(cy[c]), static_cast<int>(kArcClear));
                ++problems;
                break;
            }
        }
    }
    return problems;
}

} // namespace Audit

#endif // SIMULATOR
