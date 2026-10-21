/**
 ******************************************************************************
 * @file    Ui.cpp
 * @brief   The shared widgets (see Ui.hpp).
 ******************************************************************************
 */

#include "gui/widgets/Ui.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

#include "Layout.hpp"
#include "Types.hpp"
#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;
using namespace TrailQuest;

namespace Ui
{

uint32_t affinityInk(uint8_t affinity)
{
    switch (static_cast<Affinity>(affinity)) {
        case Affinity::Pace: return Ink::kPace;
        case Affinity::Form: return Ink::kForm;
        case Affinity::Grit: return Ink::kGrit;
        default:             return Ink::kText;
    }
}

lv_obj_t* rowLabel(lv_obj_t* parent, int32_t row, const char* text, uint32_t color)
{
    lv_obj_t* l = Draw::label(parent, &pressstart2p_16, "", 0, row, TextFit::kScreen, LV_TEXT_ALIGN_CENTER, color);
    lv_label_set_text_static(l, text);
    return l;
}

void setText(lv_obj_t* label, const char* text, uint32_t color)
{
    if (label == nullptr) {
        return;
    }
    lv_label_set_text_static(label, text != nullptr ? text : "");
    lv_obj_set_style_text_color(label, Draw::rgb(color), 0);
}

// -- Corners -------------------------------------------------------------------

void Corners::create(lv_obj_t* parent)
{
    place(parent, Layout::kTopLabelRow, Layout::kTopLabelLeft, Layout::kTopLabelRight);
}

void Corners::createForMap(lv_obj_t* parent)
{
    place(parent, Layout::kMapLabelRow, Layout::kMapLabelLeft, Layout::kMapLabelRight);
}

void Corners::place(lv_obj_t* parent, int32_t topRow, int32_t topLeft, int32_t topRight)
{
    // Each word sits by its arc, inside it: a left word starts at its row's
    // left x, a right word ends at its right x (LayoutTest checks both).
    // Stored L1, L2, R1, R2.
    const int32_t rows[2]  = { topRow, Layout::kBottomLabelRow };
    const int32_t lefts[2] = { topLeft, Layout::kBottomLabelLeft };
    const int32_t rights[2] = { topRight, Layout::kBottomLabelRight };
    constexpr int32_t kBox = 120;   // wide enough for any word; boxes are transparent
    for (int i = 0; i < 4; ++i) {
        const int  r     = i % 2;
        const bool right = i >= 2;
        mLabel[i] = Draw::label(parent, &pressstart2p_16, "", right ? rights[r] - kBox : lefts[r], rows[r], kBox,
                                right ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_LEFT, Ink::kText);
    }
}

void Corners::set(const char* l1, const char* r1, const char* l2, const char* r2)
{
    // Stored L1, L2, R1, R2 (create() makes the left pair first).
    const char*    text[4]  = { l1, l2, r1, r2 };
    const uint32_t color[4] = { Ink::kAmber, Ink::kWhite, Ink::kGreen, Ink::kRed };
    for (int i = 0; i < 4; ++i) {
        if (mLabel[i] == nullptr) {
            continue;
        }
        setText(mLabel[i], text[i], color[i]);
        Draw::setHidden(mLabel[i], text[i] == nullptr);
    }
}

void Corners::setHidden(bool hidden)
{
    for (lv_obj_t* l : mLabel) {
        if (l != nullptr && hidden) {
            Draw::setHidden(l, true);
        } else if (l != nullptr) {
            const char* t = lv_label_get_text(l);
            Draw::setHidden(l, t == nullptr || t[0] == '\0');
        }
    }
}

void Corners::raise()
{
    for (lv_obj_t* l : mLabel) {
        if (l != nullptr) {
            lv_obj_move_foreground(l);
        }
    }
}

// -- Card ------------------------------------------------------------------------

void Card::create(lv_obj_t* parent, int32_t top, int32_t bottom)
{
    if (top < 0) {
        top = Layout::kCardRow - 2;
    }
    if (bottom < 0) {
        bottom = Layout::kCardRow + (Layout::kCardLines - 1) * Layout::kCardPitch + TextFit::kGlyph + 4;
    }
    // Wide enough for thirteen characters and a margin; the big corner radius
    // keeps the corners inside L1's and R1's arcs (the edges' nearest points
    // stay within Layout::kArcClear of the centre).
    constexpr int32_t kW      = 13 * TextFit::kGlyph + 4;
    constexpr int32_t kRadius = 24;
    mPanel = Draw::box(parent, TextFit::kRadius - kW / 2, top, kW, bottom - top, Ink::kPanel, kRadius);
    lv_obj_set_style_border_color(mPanel, Draw::rgb(Ink::kEdge), 0);
    lv_obj_set_style_border_width(mPanel, 2, 0);
    // The lines are the panel's siblings, placed on the screen's rows, so
    // nothing about the panel's border or padding can move them.
    for (uint8_t i = 0; i < TextFit::kMaxLines; ++i) {
        mLine[i] = rowLabel(parent, Layout::kCardRow + i * Layout::kCardPitch, mLines.text[i]);
    }
    hide();
}

void Card::show(const TextFit::Lines& lines, uint32_t titleColor, uint32_t color)
{
    mLines = lines;
    Draw::setHidden(mPanel, false);
    lv_obj_move_foreground(mPanel);
    for (uint8_t i = 0; i < TextFit::kMaxLines; ++i) {
        setText(mLine[i], mLines.text[i], i == 0 ? titleColor : color);
        Draw::setHidden(mLine[i], i >= mLines.count || i >= Layout::kCardLines);
        lv_obj_move_foreground(mLine[i]);
    }
    mVisible = true;
}

void Card::setColor(uint8_t i, uint32_t color)
{
    if (i < TrailQuest::TextFit::kMaxLines && mLine[i] != nullptr) {
        lv_obj_set_style_text_color(mLine[i], Draw::rgb(color), 0);
    }
}

void Card::hide()
{
    if (mPanel) {
        Draw::setHidden(mPanel, true);
    }
    for (lv_obj_t* l : mLine) {
        if (l) {
            Draw::setHidden(l, true);
        }
    }
    mVisible = false;
}

// -- Bar ---------------------------------------------------------------------------

void Bar::create(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    mW     = w;
    mTrack = Draw::box(parent, x, y, w, h, 0x404040, h / 2);
    mFill  = Draw::box(parent, x, y, w, h, color, h / 2);
}

void Bar::set(uint32_t value, uint32_t max)
{
    if (mFill == nullptr) {
        return;
    }
    const uint32_t v = value > max ? max : value;
    int32_t        w = max == 0 ? 0 : static_cast<int32_t>(static_cast<uint64_t>(v) * static_cast<uint32_t>(mW) / max);
    if (v > 0 && w < 2) {
        w = 2;   // a sliver stays visible until it is truly 0
    }
    lv_obj_set_width(mFill, w);
    Draw::setHidden(mFill, w == 0);
}

void Bar::setHidden(bool hidden)
{
    Draw::setHidden(mTrack, hidden);
    Draw::setHidden(mFill, hidden || lv_obj_get_width(mFill) == 0);
}

// -- BeatBar -------------------------------------------------------------------------

void BeatBar::create(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const BeatConfig& cfg)
{
    constexpr int32_t kH = 12;
    mCfg = clampBeat(cfg);
    mX = x;
    mY = y;
    mW = w;
    mObjs[0] = Draw::box(parent, x, y, w, kH, 0x404040);
    mObjs[1] = Draw::box(parent, x, y, 1, kH, 0xC08000);
    mObjs[2] = Draw::box(parent, x, y, 1, kH, 0x00C000);
    mObjs[3] = Draw::box(parent, x, y - 3, 3, kH + 6, 0xC0C0C0);
    reconfigure(mCfg);
}

void BeatBar::reconfigure(const BeatConfig& cfg)
{
    mCfg = clampBeat(cfg);
    auto place = [&](lv_obj_t* box, uint32_t widthMs) {
        const uint32_t half = (widthMs + 1u) / 2u;
        const int32_t  x0   = mX + static_cast<int32_t>(beatToTrack(mCfg, mCfg.targetMs - half, static_cast<uint32_t>(mW)));
        const int32_t  x1   = mX + static_cast<int32_t>(beatToTrack(mCfg, mCfg.targetMs + half, static_cast<uint32_t>(mW)));
        lv_obj_set_x(box, x0);
        lv_obj_set_width(box, x1 - x0);
    };
    place(mObjs[1], mCfg.goodMs);
    place(mObjs[2], mCfg.perfectMs);
}

void BeatBar::start(uint32_t nowMs)
{
    mStartMs  = nowMs;
    mNeedleMs = nowMs;
    update(nowMs);
}

void BeatBar::update(uint32_t nowMs)
{
    mNeedleMs = nowMs;
    const uint32_t phase = beatPhaseMs(mCfg, mStartMs, nowMs);
    lv_obj_set_x(mObjs[3], mX + static_cast<int32_t>(beatToTrack(mCfg, phase, static_cast<uint32_t>(mW))) - 1);
}

BeatJudgement BeatBar::judge() const
{
    return judgeBeat(mCfg, mStartMs, mNeedleMs);
}

uint32_t BeatBar::sweeps(uint32_t nowMs) const
{
    return (nowMs - mStartMs) / mCfg.sweepMs;
}

void BeatBar::setHidden(bool hidden)
{
    for (lv_obj_t* o : mObjs) {
        if (o) {
            Draw::setHidden(o, hidden);
        }
    }
}

} // namespace Ui
