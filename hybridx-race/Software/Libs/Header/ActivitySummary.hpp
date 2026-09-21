/**
 ******************************************************************************
 * @file    ActivitySummary.hpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   The on-watch record of the last race (brief 10.2).
 ******************************************************************************
 *
 * Adapted from the SDK's Running/RunLVGL example: the track map, distances and
 * paces are gone, replaced by the per-segment list a race needs.
 *
 * Fixed-size throughout -- no std::vector -- because this is held by the
 * service for the whole life of the app and paged to the GUI a few segments at
 * a time (see CustomMessage::SummaryPage).
 *
 ******************************************************************************
 */

#ifndef ACTIVITY_SUMMARY_HPP
#define ACTIVITY_SUMMARY_HPP

#include <cstdint>
#include <ctime>

#include "RaceData.hpp"

/**
 * @brief One completed segment, as the summary remembers it.
 */
struct SegmentSummary
{
    uint8_t  type = 0u;       ///< Race::SegmentType
    uint8_t  round = 0u;      ///< 1 to 8
    uint8_t  stationId = 0u;  ///< 1 to 8, 0 when not a station
    uint32_t durationMs = 0u; ///< Active time, pauses excluded
    uint8_t  hrAvg = 0u;
    uint8_t  hrMax = 0u;
};

/**
 * @brief Everything the "Last race" screen needs.
 */
struct ActivitySummary
{
    bool valid = false;  ///< False until a race has been saved

    Race::Format format = Race::Format::Full;
    bool roxzone = false;
    bool completed = false;   ///< False when the race was ended early

    std::time_t startUtc = 0;

    uint32_t totalMs = 0u;     ///< Active time for the whole race
    uint32_t runsMs = 0u;      ///< Of which runs
    uint32_t stationsMs = 0u;  ///< Of which stations
    uint32_t roxzoneMs = 0u;   ///< Of which Roxzone, 0 when not split out

    uint8_t hrAvg = 0u;
    uint8_t hrMax = 0u;

    uint8_t count = 0u;  ///< Valid entries in @c segments
    SegmentSummary segments[Race::kMaxSegments] {};
};

#endif  // ACTIVITY_SUMMARY_HPP
