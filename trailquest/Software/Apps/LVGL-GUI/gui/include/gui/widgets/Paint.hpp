/**
 ******************************************************************************
 * @file    Paint.hpp
 * @brief   Flat shapes for LV_EVENT_DRAW_MAIN handlers (after HybridX
 *          Streak's Paint.hpp): the map is drawn by one object in one pass.
 *
 * Hard edges and flat colour on the display's 64-colour grid: shapes cost no
 * image RAM and no LVGL pool. Points are in the drawing object's own
 * coordinates; Paint adds the object's origin.
 ******************************************************************************
 */

#ifndef TRAILQUEST_PAINT_HPP
#define TRAILQUEST_PAINT_HPP

#include <cstdint>

#include "lvgl.h"

namespace Paint
{

/// The drawing context of one draw callback.
struct Canvas {
    lv_layer_t* layer;
    int32_t     ox;   ///< the object's absolute origin
    int32_t     oy;

    static Canvas of(lv_event_t* e)
    {
        auto* obj = static_cast<lv_obj_t*>(lv_event_get_current_target(e));
        lv_area_t a;
        lv_obj_get_coords(obj, &a);
        return { lv_event_get_layer(e), a.x1, a.y1 };
    }
};

inline lv_point_precise_t pt(const Canvas& c, int32_t x, int32_t y)
{
    return { static_cast<lv_value_precise_t>(c.ox + x), static_cast<lv_value_precise_t>(c.oy + y) };
}

inline void tri(const Canvas& c, int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t px, int32_t py, uint32_t color)
{
    lv_draw_triangle_dsc_t d;
    lv_draw_triangle_dsc_init(&d);
    d.color = lv_color_hex(color);
    d.opa   = LV_OPA_COVER;
    d.p[0]  = pt(c, ax, ay);
    d.p[1]  = pt(c, bx, by);
    d.p[2]  = pt(c, px, py);
    lv_draw_triangle(c.layer, &d);
}

inline void line(const Canvas& c, int32_t ax, int32_t ay, int32_t bx, int32_t by, int32_t width, uint32_t color)
{
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color       = lv_color_hex(color);
    d.opa         = LV_OPA_COVER;
    d.width       = width;
    d.round_start = 1;
    d.round_end   = 1;
    d.p1          = pt(c, ax, ay);
    d.p2          = pt(c, bx, by);
    lv_draw_line(c.layer, &d);
}

inline void disc(const Canvas& c, int32_t cx, int32_t cy, int32_t r, uint32_t color)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = lv_color_hex(color);
    d.bg_opa   = LV_OPA_COVER;
    d.radius   = LV_RADIUS_CIRCLE;
    const lv_area_t a { c.ox + cx - r, c.oy + cy - r, c.ox + cx + r, c.oy + cy + r };
    lv_draw_rect(c.layer, &d, &a);
}

inline void rect(const Canvas& c, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = lv_color_hex(color);
    d.bg_opa   = LV_OPA_COVER;
    const lv_area_t a { c.ox + x, c.oy + y, c.ox + x + w - 1, c.oy + y + h - 1 };
    lv_draw_rect(c.layer, &d, &a);
}

/// A plain, transparent object that draws itself through @p cb.
inline lv_obj_t* surface(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, lv_event_cb_t cb, void* user)
{
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_add_event_cb(o, cb, LV_EVENT_DRAW_MAIN, user);
    return o;
}

} // namespace Paint

#endif // TRAILQUEST_PAINT_HPP
