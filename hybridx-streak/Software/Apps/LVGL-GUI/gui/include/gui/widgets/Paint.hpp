/**
 ******************************************************************************
 * @file    Paint.hpp
 * @brief   Flat shapes for LV_EVENT_DRAW_MAIN handlers.
 *
 * Everything the app draws itself is triangles, lines and discs in one flat
 * colour: the panel has 64 colours and no blending, so hard edges are the
 * look (Docs/writing-a-clockface.md: "let the steps be steps"), and shapes
 * cost no image RAM. Points are in the drawing object's own coordinates;
 * Paint adds the object's origin.
 ******************************************************************************
 */

#ifndef STREAK_PAINT_HPP
#define STREAK_PAINT_HPP

#include <cstdint>

#include "lvgl.h"

#include "gui/summit/SummitGeometry.hpp"
#include "gui/theme/Theme.hpp"

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

inline void tri(const Canvas& c, Summit::Pt a, Summit::Pt b, Summit::Pt p, uint32_t color)
{
    lv_draw_triangle_dsc_t d;
    lv_draw_triangle_dsc_init(&d);
    d.color = Theme::rgb(color);
    d.opa   = LV_OPA_COVER;
    d.p[0]  = pt(c, a.x, a.y);
    d.p[1]  = pt(c, b.x, b.y);
    d.p[2]  = pt(c, p.x, p.y);
    lv_draw_triangle(c.layer, &d);
}

inline void tri(const Canvas& c, const Summit::Tri& t, uint32_t color)
{
    tri(c, t.a, t.b, t.c, color);
}

inline void line(const Canvas& c, Summit::Pt a, Summit::Pt b, int32_t width, uint32_t color)
{
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color       = Theme::rgb(color);
    d.opa         = LV_OPA_COVER;
    d.width       = width;
    d.round_start = 1;
    d.round_end   = 1;
    d.p1          = pt(c, a.x, a.y);
    d.p2          = pt(c, b.x, b.y);
    lv_draw_line(c.layer, &d);
}

inline void disc(const Canvas& c, Summit::Pt centre, int32_t r, uint32_t color)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = Theme::rgb(color);
    d.bg_opa   = LV_OPA_COVER;
    d.radius   = LV_RADIUS_CIRCLE;
    const lv_area_t a { c.ox + centre.x - r, c.oy + centre.y - r, c.ox + centre.x + r - 1, c.oy + centre.y + r - 1 };
    lv_draw_rect(c.layer, &d, &a);
}

inline void ring(const Canvas& c, Summit::Pt centre, int32_t r, int32_t width, uint32_t color)
{
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.color       = Theme::rgb(color);
    d.opa         = LV_OPA_COVER;
    d.width       = width;
    d.center      = { c.ox + centre.x, c.oy + centre.y };
    d.radius      = static_cast<uint16_t>(r);
    d.start_angle = 0;
    d.end_angle   = 360;
    lv_draw_arc(c.layer, &d);
}

inline void rect(const Canvas& c, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = Theme::rgb(color);
    d.bg_opa   = LV_OPA_COVER;
    const lv_area_t a { c.ox + x, c.oy + y, c.ox + x + w - 1, c.oy + y + h - 1 };
    lv_draw_rect(c.layer, &d, &a);
}

/// A plain, transparent object that draws itself through `cb`.
inline lv_obj_t* surface(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h,
                         lv_event_cb_t cb, void* user)
{
    lv_obj_t* o = Theme::container(parent, x, y, w, h);
    lv_obj_add_event_cb(o, cb, LV_EVENT_DRAW_MAIN, user);
    return o;
}

} // namespace Paint

#endif // STREAK_PAINT_HPP
