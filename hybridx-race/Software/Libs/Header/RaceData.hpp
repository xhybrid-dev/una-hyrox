/**
 ******************************************************************************
 * @file    RaceData.hpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   HYROX reference data: station order, distances and reps.
 ******************************************************************************
 *
 * The single place the race format is defined, so a rule change is a one-file
 * edit (brief 7.1). Confirmed by Jon against the HybridX data pack v2.2 on
 * 21 September 2026.
 *
 * Station weights are deliberately absent: the app never displays them.
 *
 ******************************************************************************
 */

#ifndef RACE_DATA_HPP
#define RACE_DATA_HPP

#include <cstddef>
#include <cstdint>

namespace Race
{

/**
 * @brief Kind of segment.
 *
 * The values are written into the FIT lap's @c segment_type developer field
 * (brief 10.1) and must not be renumbered.
 */
enum class SegmentType : uint8_t
{
    Run = 0,      ///< The 1 km run before a station
    RoxIn = 1,    ///< Roxzone entry, only when Roxzone splitting is on
    Station = 2,  ///< The station itself
    RoxOut = 3,   ///< Roxzone exit, only when Roxzone splitting is on
};

/**
 * @brief Race format.
 *
 * The values are written into the FIT session's @c race_format developer field
 * (brief 10.1) and must not be renumbered.
 */
enum class Format : uint8_t
{
    Full = 0,   ///< All eight rounds
    HalfA = 1,  ///< Rounds 1 to 4
    HalfB = 2,  ///< Rounds 5 to 8
};

/**
 * @brief One station of the race.
 *
 * @c work is what the watch shows next to the name, already formatted with its
 * unit, because the app never computes with it.
 */
struct Station
{
    const char *name;   ///< Display name, British English, upper case
    const char *work;   ///< Work as shown, e.g. "50 m" or "100 reps"
    const char *brief;  ///< Name for the split list, where 240 px is round
};

/// Number of stations in a full race.
constexpr uint8_t kStationCount = 8;

/// Number of runs in a full race; also the denominator in "RUN 3/8".
constexpr uint8_t kRunCount = 8;

/// Work shown for every run.
constexpr const char *kRunWork = "1 km";

/**
 * @brief Separator between a segment's name and its work, as brief 7.2 writes it:
 *        "SLED PULL \u00b7 50 m".
 *
 * U+00B7 is outside printable ASCII. The fonts inherited from the Run app were
 * ASCII only, so this first rendered as an empty box; 0xB7 is now in the range
 * in LVGL-GUI/assets/gen_assets.py and the text faces carry the glyph.
 */
constexpr const char *kLabelSep = "\xC2\xB7";

/**
 * @brief The HYROX 26/27 station order.
 *
 * Indexed 0 to 7; station IDs elsewhere are 1-based, so station @c id lives at
 * @c kStations[id - 1].
 */
constexpr Station kStations[kStationCount] = {
    { "SKIERG",             "1000 m",   "SKIERG"     },
    { "SLED PUSH",          "50 m",     "SLED PUSH"  },
    { "SLED PULL",          "50 m",     "SLED PULL"  },
    { "BURPEE BROAD JUMPS", "80 m",     "BURPEES"    },
    { "ROW",                "1000 m",   "ROW"        },
    { "FARMERS CARRY",      "200 m",    "CARRY"      },
    { "SANDBAG LUNGES",     "100 m",    "LUNGES"     },
    { "WALL BALLS",         "100 reps", "WALL BALLS" },
};

/**
 * @brief Largest segment list any format can produce.
 *
 * A full race with Roxzone splitting on: four segments per round for eight
 * rounds, less the ROX_OUT that never follows the final station (brief 7.2).
 * Every buffer sized from this is checked before use -- there is no MMU.
 */
constexpr uint8_t kMaxSegments = 31;

/**
 * @brief Buffer size that always holds a formatted segment label.
 *
 * The longest is "BURPEE BROAD JUMPS \xC2\xB7 80 m": 26 bytes with the UTF-8
 * middle dot, plus a terminator. Rounded up for headroom.
 */
constexpr size_t kMaxLabelLen = 32;

/**
 * @brief Buffer size that always holds a segment name without its work.
 *
 * The race face shows the name and the work on separate lines, because the
 * one-line form of brief 7.2 overruns a round 240 px display. The longest name
 * is "BURPEE BROAD JUMPS": 18 bytes plus a terminator.
 */
constexpr size_t kMaxNameLen = 20;

/**
 * @brief Width budget for a split-list row, in characters.
 *
 * The summary's split rows sit near the bottom of a round display, where the
 * chord is about 175 px wide -- roughly eleven characters of the face they are
 * drawn in, once the time has taken its share. @c Station::brief exists for
 * that row and nothing else; @c Station::name is what every other screen, the
 * FIT lap name and the summary heading use.
 *
 * The abbreviations are display text, not HYROX terminology: Jon signs them off
 * at Gate 4 (see NOTES.md 4.4).
 */
constexpr size_t kMaxBriefLen = 12;

/**
 * @brief First round of a format.
 *
 * @param format Race format.
 * @retval 1, or 5 for the second half.
 */
constexpr uint8_t firstRound(Format format)
{
    return (format == Format::HalfB) ? 5u : 1u;
}

/**
 * @brief Last round of a format.
 *
 * @param format Race format.
 * @retval 4 for the first half, otherwise 8.
 */
constexpr uint8_t lastRound(Format format)
{
    return (format == Format::HalfA) ? 4u : 8u;
}

}  // namespace Race

#endif  // RACE_DATA_HPP
