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
    const char *name;       ///< Display name, British English, upper case
    const char *work;       ///< Work as shown, e.g. "50 m" or "100 reps"
    const char *brief;      ///< Name for the split list, where 240 px is round
    uint16_t    distanceM;  ///< Metres credited to the FIT lap; 0 when the work is reps
};

/// Number of stations in a full race.
constexpr uint8_t kStationCount = 8;

/// Number of runs in a full race; also the denominator in "RUN 3/8".
constexpr uint8_t kRunCount = 8;

/**
 * @brief Run distance bounds, in metres.
 *
 * A HYROX run is 1 km and that is the default, but a training sim is often run
 * shorter -- 500 m or 800 m is a common test piece -- so the distance is
 * adjustable in 100 m steps (Jon's request, 23 September 2026). The ceiling is
 * the race distance: this exists to make a sim shorter, not to invent a longer
 * HYROX.
 */
constexpr uint16_t kRunDistanceStepM = 100u;
constexpr uint16_t kRunDistanceMinM = 100u;
constexpr uint16_t kRunDistanceMaxM = 1000u;
constexpr uint16_t kRunDistanceDefaultM = 1000u;

/// Number of selectable run distances: 100 m to 1000 m inclusive.
constexpr uint8_t kRunDistanceCount =
        static_cast<uint8_t>((kRunDistanceMaxM - kRunDistanceMinM) / kRunDistanceStepM + 1u);

/**
 * @brief Work shown for a run, indexed by hundreds of metres.
 *
 * A table rather than a formatter: the watch formats with integers only
 * (brief 14.4), every caller wants a @c const @c char* it does not own, and
 * there are only ten of them. @c kRunWorkByHundreds[10] is the race distance and
 * reads "1 km" rather than "1000 m", which is how the format writes it.
 */
constexpr const char *kRunWorkByHundreds[kRunDistanceCount + 1u] = {
    "",      "100 m", "200 m", "300 m", "400 m",
    "500 m", "600 m", "700 m", "800 m", "900 m", "1 km",
};

/**
 * @brief Clamp a run distance to the selectable range, rounded down to a step.
 *
 * @param metres Requested distance.
 * @retval A value between kRunDistanceMinM and kRunDistanceMaxM, on a step.
 */
constexpr uint16_t clampRunDistanceM(uint16_t metres)
{
    if (metres < kRunDistanceMinM) {
        return kRunDistanceMinM;
    }
    if (metres > kRunDistanceMaxM) {
        return kRunDistanceMaxM;
    }
    return static_cast<uint16_t>((metres / kRunDistanceStepM) * kRunDistanceStepM);
}

/**
 * @brief The work text for a run of @p metres.
 *
 * @param metres Run distance; clamped, so a corrupt value cannot index out.
 * @retval A string literal, never nullptr.
 */
constexpr const char *runWork(uint16_t metres)
{
    return kRunWorkByHundreds[clampRunDistanceM(metres) / kRunDistanceStepM];
}

/**
 * @brief How the FIT file credits distance.
 *
 * Jon's decision, 22 September 2026: the file carries every distance the format
 * states, so a full race at the default run distance totals 10 480 m -- eight
 * kilometres of running plus the stations' own metres, the SkiErg's and the
 * Row's included even though those are machine metres rather than ground
 * covered. Wall Balls are reps and carry nothing.
 *
 * This is what fills in Distance and Avg Pace on Garmin Connect and Strava;
 * without it every one of those fields reads "--" (NOTES.md 5.9). The run part
 * of it moves with the configured run distance (NOTES.md 5.13).
 */

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
    { "SKIERG",             "1000 m",   "SKIERG",     1000u },
    { "SLED PUSH",          "50 m",     "SLED PUSH",    50u },
    { "SLED PULL",          "50 m",     "SLED PULL",    50u },
    { "BURPEE BROAD JUMPS", "80 m",     "BURPEES",      80u },
    { "ROW",                "1000 m",   "ROW",        1000u },
    { "FARMERS CARRY",      "200 m",    "CARRY",       200u },
    { "SANDBAG LUNGES",     "100 m",    "LUNGES",      100u },
    { "WALL BALLS",         "100 reps", "WALL BALLS",    0u },
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
