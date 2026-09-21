/**
 ******************************************************************************
 * @file    Track.hpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   Track namespace: activity state and the live race snapshot.
 ******************************************************************************
 *
 * Adapted from the SDK's Running/RunLVGL example. "Track" is kept as the name
 * for the activity being recorded, as the SDK's activity apps use it, but the
 * contents are now a race rather than a run: no pace, no distance, no speed,
 * no elevation, no interval phase machine.
 *
 * @c Data is what the GUI needs once a second and on every split. It is
 * deliberately small and fixed-size so it fits a kernel message pool block
 * (256 bytes -- see the static_assert in Commands.hpp).
 *
 ******************************************************************************
 */

#ifndef TRACK_HPP
#define TRACK_HPP

#include <cstdint>

#include "RaceData.hpp"
#include "RaceModel.hpp"

namespace Track
{

/**
 * @brief Activity tracking state, as the GUI sees it.
 *
 * A reduction of Race::RaceModel::State: the GUI does not care whether a
 * finished race has been saved yet, only whether the clock is moving.
 */
enum class State
{
    INACTIVE = 0,  ///< No race, or the race is over
    ACTIVE,        ///< Clock advancing
    PAUSED         ///< Clock held
};

/**
 * @brief Live race snapshot, sent at 1 Hz and on every split.
 *
 * Times are milliseconds. Heart rate is bpm. The GUI formats; it never
 * computes.
 */
struct Data
{
    // -- Where we are --------------------------------------------------------
    Race::SegmentDesc current {};   ///< The open segment
    Race::SegmentDesc next {};      ///< The one after it; only valid if hasNext
    uint8_t segmentIndex = 0u;      ///< 0-based index of the open segment
    uint8_t segmentCount = 0u;      ///< Segments in the whole race
    bool    hasNext = false;        ///< False on the final segment

    // -- Clocks, ms ----------------------------------------------------------
    uint32_t segmentMs = 0u;   ///< Active time in the open segment
    uint32_t totalMs = 0u;     ///< Active time across the race
    uint32_t elapsedMs = 0u;   ///< Wall time since the start, pauses included

    // -- Heart rate, bpm -----------------------------------------------------
    uint8_t hr = 0u;         ///< Live value, shown unconditionally (brief 14.12)
    uint8_t hrTrust = 0u;    ///< Trust level as reported by HEART_RATE_EX
    uint8_t hrSource = 0u;   ///< 0 none, 1 optical, 2 external strap
    uint8_t hrAvg = 0u;      ///< Mean over the race so far
    uint8_t hrMax = 0u;      ///< Peak over the race so far

    // -- Flags ---------------------------------------------------------------
    bool completed = false;  ///< True once the final split has landed
};

/**
 * @brief What a finished segment looked like, for the split toast (brief 8.3).
 */
struct SplitEvent
{
    Race::SegmentDesc desc {};  ///< The segment that just ended
    uint8_t  index = 0u;        ///< Its 0-based index
    uint32_t activeMs = 0u;     ///< Its active duration
    bool     raceFinished = false;  ///< True when this split ended the race
};

}  // namespace Track

#endif  // TRACK_HPP
