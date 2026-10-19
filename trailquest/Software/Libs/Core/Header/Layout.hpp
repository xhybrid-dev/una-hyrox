/**
 ******************************************************************************
 * @file    Layout.hpp
 * @brief   Where text goes on the round face, and how much of it fits there.
 *
 * Header-only: the GUI places its labels at these rows, and the host tests
 * (LayoutTest.cpp) check each row really holds its column count on a 240 px
 * circle (TextFit.hpp), and that everything the game says fits its block.
 *
 * Every screen shares one plan, so the buttons are always in the same place:
 *
 *   row  28   the headline: the clock, a page title, the spirit's numbers
 *   row  56   L1's word at the left, R1's at the right (inside their arcs)
 *   rows 72+  a card: a title (12 characters), then four lines of 13
 *   row 160   L2's word at the left, R2's at the right
 *   row 176   the map's chosen exit
 *   row 190   the duel's runner numbers
 *   row 192   on the map: what lies that way
 *
 * The map moves its top labels up (row 42, under the clock at row 24) to
 * leave the middle of the face to the trail.
 *
 * A story page has its headline on row 28 and its body on the card's rows,
 * five lines of twelve (no title line), over a strip of scene (rows 156 on).
 *
 * The duel's narration takes the label rows while a turn plays back (no
 * button does anything then), two lines of twelve.
 ******************************************************************************
 */

#ifndef TRAILQUEST_LAYOUT_HPP
#define TRAILQUEST_LAYOUT_HPP

#include <cstdint>

namespace TrailQuest::Layout
{

constexpr int32_t kHeadRow  = 28;
constexpr uint8_t kHeadCols = 9;

/// The corner words sit inside the bezel arcs (SDK Buttons: radius 113, 6 px
/// wide, so the inner edge is at 110): every corner of a label's box stays
/// within kArcClear of the centre. A left word starts at its row's Left x; a
/// right word ends at its row's Right x.
constexpr int32_t kArcClear = 109;

constexpr int32_t kTopLabelRow      = 56;
constexpr int32_t kTopLabelLeft     = 34;
constexpr int32_t kTopLabelRight    = 206;
constexpr int32_t kBottomLabelRow   = 160;
constexpr int32_t kBottomLabelLeft  = 30;
constexpr int32_t kBottomLabelRight = 210;

/// A card: a title line, then four of thirteen. Thirteen characters clear
/// the bezel arcs only on rows 87-137; the title row, nearer L1's and R1's
/// arcs, takes twelve.
constexpr int32_t kCardRow       = 72;
constexpr int32_t kCardPitch     = 16;
constexpr uint8_t kCardLines     = 5;
constexpr uint8_t kCardCols      = 13;
constexpr uint8_t kCardTitleCols = 12;

constexpr int32_t kNarrRow   = 56;
constexpr int32_t kNarrPitch = 20;   // a gap between the two rows (the labels' and the technique's)
constexpr uint8_t kNarrLines = 2;
constexpr uint8_t kNarrCols  = 12;

constexpr int32_t kExitRow  = 176;
constexpr uint8_t kExitCols = 10;

constexpr int32_t kFootRow  = 190;
constexpr uint8_t kFootCols = 10;

/// A story page's body: on the card's rows, every line as narrow as the
/// title row's.
constexpr int32_t kStoryRow   = kCardRow;
constexpr uint8_t kStoryLines = kCardLines;
constexpr uint8_t kStoryCols  = kCardTitleCols;
/// The scene under it: the sky from kSceneTop, the ground from kSceneGround,
/// which the figures stand on.
constexpr int32_t kSceneTop    = 156;
constexpr int32_t kSceneGround = 216;

constexpr int32_t kMapClockRow  = 24;
constexpr uint8_t kMapClockCols = 8;
constexpr int32_t kMapLabelRow   = 42;
constexpr int32_t kMapLabelLeft  = 44;
constexpr int32_t kMapLabelRight = 196;
/// The map's places are drawn between these rows (icons included).
constexpr int32_t kMapTop    = 60;
constexpr int32_t kMapBottom = 174;
/// Under the chosen way's name (kExitRow): what lies that way.
constexpr int32_t kLeadsRow  = 192;
constexpr uint8_t kLeadsCols = 9;
/// The runner's stamina, under that.
constexpr int32_t kMapBarRow = 212;

} // namespace TrailQuest::Layout

#endif // TRAILQUEST_LAYOUT_HPP
