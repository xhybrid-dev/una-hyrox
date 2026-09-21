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
    const char *name;  ///< Display name, British English, upper case
    const char *work;  ///< Work as shown, e.g. "50 m" or "100 reps"
};

/// Number of stations in a full race.
constexpr uint8_t kStationCount = 8;

/// Number of runs in a full race; also the denominator in "RUN 3/8".
constexpr uint8_t kRunCount = 8;

/// Work shown for every run.
constexpr const char *kRunWork = "1 km";

/**
 * @brief Separator between a segment's name and its work, e.g. "SLED PULL - 50 m".
 *
 * Brief 7.2 shows a middle dot. The shipped Poppins subsets are ASCII only
 * (0x20-0x7E, see LVGL-GUI/assets/gen_assets.py), so U+00B7 rendered as an
 * empty box on the first simulator run. A hyphen is used until the fonts are
 * regenerated with the glyph in Phase 4; that needs lv_font_conv, which is a
 * Node tool and an asset job rather than a code change.
 */
constexpr const char *kLabelSep = "-";

/**
 * @brief The HYROX 26/27 station order.
 *
 * Indexed 0 to 7; station IDs elsewhere are 1-based, so station @c id lives at
 * @c kStations[id - 1].
 */
constexpr Station kStations[kStationCount] = {
    { "SKIERG",             "1000 m"   },
    { "SLED PUSH",          "50 m"     },
    { "SLED PULL",          "50 m"     },
    { "BURPEE BROAD JUMPS", "80 m"     },
    { "ROW",                "1000 m"   },
    { "FARMERS CARRY",      "200 m"    },
    { "SANDBAG LUNGES",     "100 m"    },
    { "WALL BALLS",         "100 reps" },
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
