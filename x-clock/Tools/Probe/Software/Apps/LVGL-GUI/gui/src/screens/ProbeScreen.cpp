/**
 ******************************************************************************
 * @file    ProbeScreen.cpp
 * @brief   The Clock Probe's screen (see the header).
 ******************************************************************************
 */

#include "gui/screens/ProbeScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

namespace Draw  = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;
using namespace ClockProbe;

namespace
{
// The ring: centre-line radius 110 and 8 px thick, so it spans 106 to 114 on the 120 px disc.
constexpr int32_t kRingRadius = 110;
constexpr int32_t kRingWidth  = 8;
constexpr int32_t kMarkRadius = 117;   ///< today's mark, outside the ring
constexpr int32_t kMarkWidth  = 3;
/// 360 / 7 degrees a day, less a gap of 6, in whole degrees (lv_arc takes ints).
constexpr int32_t dayStart(int32_t i) { return (i * 360) / 7 + 3; }
constexpr int32_t dayEnd(int32_t i) { return ((i + 1) * 360) / 7 - 3; }

// Rows: a dot at x 34, text from x 42, 18 px apart. A round face is widest in the middle, but the
// rows sit within 54 px of the centre, where the chord is still about 160 px.
constexpr int32_t kRowsY    = 60;
constexpr int32_t kRowStep  = 18;
constexpr int32_t kDotX     = 34;
constexpr int32_t kTextX    = 42;
constexpr int32_t kTextW    = 160;
constexpr int32_t kTrendRight = 204;   ///< the VO2max bars end here
constexpr int32_t kBarW     = 3;
constexpr int32_t kBarStep  = 4;
constexpr int32_t kBarMaxH  = 14;

uint32_t ringColor(uint8_t level)
{
    switch (static_cast<Level>(level)) {
        case Level::High: return Color::YELLOW;        // the HybridX yellow
        case Level::Low:  return Color::YELLOW_DARK;   // amber
        default:          return Color::GRAY_DARK;     // none
    }
}

uint32_t lightColor(uint8_t light)
{
    switch (static_cast<Light>(light)) {
        case Light::Green: return Color::GREEN;
        case Light::Amber: return Color::YELLOW_DARK;
        case Light::Red:   return Color::RED;
        default:           return Color::GRAY;
    }
}
} // namespace

ProbeScreen::ProbeScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);

    for (int32_t i = 0; i < kRingDays; ++i) {
        mDay[i] = Draw::arc(mRoot, SDK::LVGL::kCx, SDK::LVGL::kCy, kRingRadius, kRingWidth, dayStart(i), dayEnd(i),
                            Color::GRAY_DARK, false);
    }
    mToday = Draw::arc(mRoot, SDK::LVGL::kCx, SDK::LVGL::kCy, kMarkRadius, kMarkWidth, dayStart(0), dayEnd(0),
                       Color::WHITE, false);
    Draw::setHidden(mToday, true);

    mTime = Draw::label(mRoot, &poppins_semibold_25, "--:--", 40, 20, 160, LV_TEXT_ALIGN_CENTER, Color::WHITE);

    for (uint8_t i = 0; i < kRows; ++i) {
        const int32_t y = kRowsY + i * kRowStep;
        mDot[i] = Draw::dot(mRoot, kDotX, y + 11, 3, Color::GRAY);
        mRow[i] = Draw::label(mRoot, &poppins_regular_14, "", kTextX, y, kTextW, LV_TEXT_ALIGN_LEFT, Color::WHITE);
    }
    for (uint8_t i = 0; i < kTrendMax; ++i) {
        mBar[i] = Draw::box(mRoot, kTrendRight - kBarW, kRowsY + (kRows - 1) * kRowStep + 18, kBarW, 2, Color::TEAL);
        Draw::setHidden(mBar[i], true);
    }
    mFoot = Draw::label(mRoot, &poppins_regular_14, "Probe 0.1.0", 60, 196, 120, LV_TEXT_ALIGN_CENTER, Color::GRAY_DARK);

    mModel.bind(this);
    draw();
}

ProbeScreen::~ProbeScreen()
{
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void ProbeScreen::onChanged()
{
    draw();
}

void ProbeScreen::draw()
{
    const ClockData& c = mModel.clock();
    char t[16];
    if (c.valid) {
        if (c.is12h) {
            const unsigned h = c.hour % 12u == 0 ? 12u : c.hour % 12u;
            std::snprintf(t, sizeof(t), "%u:%02u%s", h, static_cast<unsigned>(c.minute), c.hour >= 12 ? "pm" : "am");
        } else {
            std::snprintf(t, sizeof(t), "%02u:%02u", static_cast<unsigned>(c.hour), static_cast<unsigned>(c.minute));
        }
    } else {
        std::snprintf(t, sizeof(t), "--:--");
    }
    lv_label_set_text(mTime, t);

    if (!mModel.haveView()) {
        return;
    }
    const ViewData& v = mModel.view();

    for (int32_t i = 0; i < kRingDays; ++i) {
        Draw::setArcColor(mDay[i], ringColor(v.level[i]));
    }
    if (v.today >= 0 && v.today < kRingDays) {
        Draw::setArc(mToday, dayStart(v.today), dayEnd(v.today));
        Draw::setHidden(mToday, false);
    } else {
        Draw::setHidden(mToday, true);
    }

    for (uint8_t i = 0; i < kRows; ++i) {
        lv_label_set_text(mRow[i], v.rows[i].text);
        lv_obj_set_style_bg_color(mDot[i], Draw::rgb(lightColor(v.rows[i].light)), LV_PART_MAIN);
    }

    // The VO2max trend, drawn to the right of its row, right-aligned, oldest on the left.
    const int32_t base = kRowsY + (kRows - 1) * kRowStep + 17;
    for (uint8_t i = 0; i < kTrendMax; ++i) {
        if (i < v.trendCount) {
            const int32_t h = (v.trend[i] * kBarMaxH + 19) / 20;
            const int32_t x = kTrendRight - (v.trendCount - i) * kBarStep + (kBarStep - kBarW);
            lv_obj_set_pos(mBar[i], x, base - h);
            lv_obj_set_size(mBar[i], kBarW, h);
            Draw::setHidden(mBar[i], false);
        } else {
            Draw::setHidden(mBar[i], true);
        }
    }
}
