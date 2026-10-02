/**
 ******************************************************************************
 * @file    Track.hpp
 * @date    08-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Track namespace: state, interval types and real-time metrics.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef TRACK_HPP
#define TRACK_HPP

#include <cstdint>
#include <ctime>

namespace Track
{

/**
 * @brief Activity tracking state.
 */
enum class State {
    INACTIVE = 0,
    ACTIVE,
    PAUSED
};

// =============================================================================
// Interval training types
// =============================================================================

/**
 * @brief Current phase of an interval training session.
 */
enum class IntervalsPhase : uint8_t {
    WARM_UP   = 0,
    RUN,
    REST,
    COOL_DOWN
};

/**
 * @brief Timer display mode for the current interval phase.
 *
 * Determined by the workout configuration; the Service sets this field so
 * the GUI never needs to deduce it.
 */
enum class IntervalsMetric : uint8_t {
    TIME_OPEN       = 0, ///< Count-up, no fixed target     — label "Open"  (default / WARM_UP / COOL_DOWN)
    TIME_REMAINING,      ///< Countdown to phase end         — label "Remaining"
    TIME_ELAPSED,        ///< Count-up from phase start      — label "Elapsed"
    DISTANCE             ///< Distance remaining to phase end — label "km/mi remaining"
};

/**
 * @brief Snapshot of interval training state; updated every tick by the Service.
 *
 * All distance values are in metres; the GUI converts to km/mi as needed.
 * phaseTimerSec is remaining / elapsed / open time depending on metric.
 */
struct IntervalsData {
    IntervalsPhase  phase         = IntervalsPhase::WARM_UP;
    IntervalsMetric metric        = IntervalsMetric::TIME_OPEN;
    uint8_t         repeat        = 0;    ///< 1-based current repeat index
    uint8_t         totalRepeats  = 0;    ///< Total number of RUN-REST cycles
    std::time_t     phaseTimerSec = 0;    ///< Seconds (meaning depends on metric)
    float           distRemaining = 0.0f; ///< Metres remaining (DISTANCE metric only)

    // HybridX Intervals: the workout step behind the phase (Core/WorkoutRunner).
    // Plain types, so the GUI needs no Core header to read them; the enums'
    // values are Intervals::TargetKind and Intervals::ZoneState.
    uint8_t  stepIndex   = 0;     ///< in the workout file's step list (and FIT workout_step index)
    int16_t  nextIndex   = -1;    ///< the step after this one; -1 if this is the last
    uint16_t passes      = 0;     ///< repeat block size; 0 outside a block (repeat/totalRepeats hold the same, capped at 255)
    uint8_t  targetKind  = 0;     ///< Intervals::TargetKind: 0 open, 1 pace, 2 HR zone, 3 HR bpm
    uint16_t targetLow   = 0;     ///< pace: the fast bound, s/km; HR: the low bound
    uint16_t targetHigh  = 0;
    uint8_t  zone        = 0;     ///< Intervals::ZoneState, debounced (for colour)
    uint8_t  zoneLive    = 0;     ///< Intervals::ZoneState, this second's reading
    bool     settling    = false; ///< the step began less than WorkoutRunner::kSettleMs ago
};

// =============================================================================

/**
 * @brief Real-time metrics snapshot; updated every second by the Service.
 *
 * All distances are in metres, speeds in m/s, pace in s/m, times in seconds.
 * The GUI converts to display units as needed.
 *
 * @note This snapshot exists for the GUI only. The live speed and pace it
 *       carries are smoothed for readability; the raw samples still back the
 *       FIT records, session averages and maxima.
 */
struct Data {

    // Pace, s/m
    float pace        = 0.0f;   ///< Live pace, smoothed over a rolling window (see SpeedSmoother)
    float avgPace     = 0.0f;
    float lapPace     = 0.0f;

    // Distance, m
    float distance    = 0.0f;
    float lapDistance = 0.0f;

    // Time, s
    std::time_t totalTime = 0;
    std::time_t lapTime   = 0;

    uint32_t lapNum = 0;

    // Heart rate, bpm
    float hr            = 0.0f;
    float hrTrustLevel  = 0.0f;
    uint8_t hrSource    = 0;     // SDK HeartRate::Source: 0 none/unknown, 1 optical, 2 external
    float avgHR         = 0.0f;
    float maxHR         = 0.0f;
    float avgLapHR      = 0.0f;
    float maxLapHR      = 0.0f;

    // Speed, m/s
    float speed        = 0.0f;   ///< Live speed, smoothed over the same window as pace
    float avgSpeed     = 0.0f;
    float maxSpeed     = 0.0f;
    float avgLapSpeed  = 0.0f;
    float maxLapSpeed  = 0.0f;

    // Elevation, m (current altitude from barometer)
    float elevation    = 0.0f;

    bool intervalsMode = false; ///< true when the track was started in intervals mode

    // Interval training (populated only when intervalsMode == true)
    IntervalsData intervals {};
};

} // namespace Track

#endif // TRACK_HPP
