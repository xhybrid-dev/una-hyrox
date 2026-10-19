/**
 ******************************************************************************
 * @file    Ui.hpp
 * @brief   The few widgets every screen is made of, placed on the rows of
 *          Layout.hpp (which the host tests check against the round face).
 *
 *   - row labels: one centred line in the game's font;
 *   - Corners: the four words by the buttons' arcs;
 *   - Card: a dark panel with up to five lines (TextFit::Lines);
 *   - Bar: a stamina bar;
 *   - BeatBar: the timing beat (the Q0 probe's, judged in the GUI with the
 *     core's Beat.hpp, against the needle on screen).
 *
 * All text is set with lv_label_set_text_static from buffers that live as
 * long as the widget (the Q0 review: lv_label_set_text allocates in LVGL's
 * pool on every change).
 ******************************************************************************
 */

#ifndef TRAILQUEST_UI_HPP
#define TRAILQUEST_UI_HPP

#include <cstdint>

#include "lvgl.h"

#include "Beat.hpp"
#include "TextFit.hpp"

namespace Ui
{

/// The game's palette for text and shapes (all on the display's grid).
namespace Ink
{
constexpr uint32_t kText   = 0xC0C0C0;   ///< white
constexpr uint32_t kDim    = 0x808080;   ///< grey
constexpr uint32_t kAmber  = 0xC08000;   ///< L1: choose
constexpr uint32_t kGreen  = 0x00C000;   ///< R1: confirm
constexpr uint32_t kWhite  = 0xC0C0C0;   ///< L2: back
constexpr uint32_t kRed    = 0xC00000;   ///< R2: leave, or the spirit's side
constexpr uint32_t kGold   = 0xC0C000;   ///< good news
constexpr uint32_t kPanel  = 0x000040;   ///< a card's background (the sprites' outline navy)
constexpr uint32_t kEdge   = 0x404080;   ///< a card's border
constexpr uint32_t kPace   = 0xC08000;   ///< affinity colours
constexpr uint32_t kForm   = 0x4080C0;
constexpr uint32_t kGrit   = 0xC04040;
} // namespace Ink

uint32_t affinityInk(uint8_t affinity);

/// A label in the game's font, centred across the face, at @p row. Its text
/// is @p text (static: the caller's buffer must outlive it).
lv_obj_t* rowLabel(lv_obj_t* parent, int32_t row, const char* text, uint32_t color = Ink::kText);

/// Set a label's text (static) and colour.
void setText(lv_obj_t* label, const char* text, uint32_t color);

/// The four words by the buttons' arcs: L1 and R1 on the top row, L2 and R2 on
/// the bottom, inside the arcs (Layout.hpp). nullptr hides one.
class Corners
{
public:
    /// The usual rows (Layout::kTopLabelRow, kBottomLabelRow).
    void create(lv_obj_t* parent);
    /// The map's: its top pair higher up (Layout::kMapLabelRow).
    void createForMap(lv_obj_t* parent);
    void set(const char* l1, const char* r1, const char* l2, const char* r2);
    void setHidden(bool hidden);
    /// Above everything else (a card's panel, shown later, overlaps the top
    /// pair's descenders by its border).
    void raise();

private:
    void place(lv_obj_t* parent, int32_t topRow, int32_t topLeft, int32_t topRight);
    lv_obj_t* mLabel[4] = {};
};

/// A dark panel with up to TextFit::kMaxLines lines on the card rows.
class Card
{
public:
    void create(lv_obj_t* parent, int32_t top = -1, int32_t bottom = -1);
    /// Show @p lines; the first in @p titleColor, the rest in @p color.
    void show(const TrailQuest::TextFit::Lines& lines, uint32_t titleColor = Ink::kGold, uint32_t color = Ink::kText);
    void hide();
    bool visible() const { return mVisible; }
    /// Recolour line @p i of the card on show.
    void setColor(uint8_t i, uint32_t color);

private:
    lv_obj_t*                   mPanel = nullptr;
    lv_obj_t*                   mLine[TrailQuest::TextFit::kMaxLines] = {};
    TrailQuest::TextFit::Lines  mLines;
    bool                        mVisible = false;
};

/// A stamina bar: a dark track and a fill.
class Bar
{
public:
    void create(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    void set(uint32_t value, uint32_t max);
    void setHidden(bool hidden);

private:
    lv_obj_t* mTrack = nullptr;
    lv_obj_t* mFill  = nullptr;
    int32_t   mW     = 0;
};

/// The timing beat: a track, the good and perfect zones, a needle.
class BeatBar
{
public:
    /// @p cfg is the beat for this stage (TrailQuest::beatFor): what is drawn
    /// is what judge() judges.
    void create(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, const TrailQuest::BeatConfig& cfg);
    /// Start sweeping from @p nowMs (lv_tick_get()).
    void start(uint32_t nowMs);
    /// Move the needle to @p nowMs. Call once a frame.
    void update(uint32_t nowMs);
    /// Judge a press against the needle the player saw (the last update()'s
    /// time: a button code arrives before the frame is redrawn).
    TrailQuest::BeatJudgement judge() const;
    /// Whole sweeps since start().
    uint32_t sweeps(uint32_t nowMs) const;
    void setHidden(bool hidden);

private:
    TrailQuest::BeatConfig mCfg = TrailQuest::clampBeat(TrailQuest::BeatConfig{});   // until create()
    lv_obj_t* mObjs[4] = {};   ///< track, good, perfect, needle
    int32_t   mX = 0;
    int32_t   mW = 0;
    uint32_t  mStartMs  = 0;
    uint32_t  mNeedleMs = 0;
};

} // namespace Ui

#endif // TRAILQUEST_UI_HPP
