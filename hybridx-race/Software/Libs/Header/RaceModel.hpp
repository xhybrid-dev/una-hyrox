/**
 ******************************************************************************
 * @file    RaceModel.hpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   Race state machine and timekeeping. Pure C++, no SDK dependencies.
 ******************************************************************************
 *
 * Implements brief 7.2 to 7.5. Deliberately knows nothing about the watch: no
 * SDK headers, no heap, no floats, no std::string. That is what lets a
 * 90-minute race be simulated instantly in a host test.
 *
 * Time is passed in, never read. Brief 7.4 requires a split to be stamped at
 * the moment the button is pressed in the GUI, so a model that called its own
 * clock would re-time every split by up to one GUI tick (100 ms). Every method
 * that needs "now" takes it as a uint32_t of monotonic milliseconds, and every
 * comparison is an unsigned subtraction so the kernel's wrapping clock is
 * handled (brief 7.4, 14.6).
 *
 * Illegal transitions are no-ops returning false, never assertions: a stray
 * press must not be able to corrupt a race in progress.
 *
 ******************************************************************************
 */

#ifndef RACE_MODEL_HPP
#define RACE_MODEL_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "RaceData.hpp"

namespace Race
{

/**
 * @brief One planned segment: what the athlete is about to do.
 */
struct SegmentDesc
{
    SegmentType type;   ///< Run, Roxzone in/out, or station
    uint8_t     round;  ///< Real round number, 1 to 8, even for a half race
    uint8_t     stationId;  ///< Station ID 1 to 8, or 0 when not a station
};

/**
 * @brief One completed segment: what the athlete actually did.
 *
 * The heart-rate accumulators are carried here rather than derived later so
 * that merging two segments on an undo is exact (brief 10.1): a sum and a
 * count merge losslessly where two averages would not.
 */
struct SegmentResult
{
    SegmentDesc desc;      ///< The plan this segment came from
    uint32_t    startMs;   ///< Monotonic tick when the segment opened
    uint32_t    activeMs;  ///< Time in the segment, excluding pauses
    uint32_t    pausedMs;  ///< Time spent paused inside the segment
    uint32_t    hrSum;     ///< Sum of heart-rate samples taken in the segment
    uint16_t    hrCount;   ///< Number of samples in @c hrSum
    uint8_t     hrMax;     ///< Highest sample seen, 0 if none

    /**
     * @brief Mean heart rate over the segment.
     *
     * @retval Rounded bpm, or 0 when no samples were taken.
     */
    uint8_t hrAvg() const
    {
        if (hrCount == 0u) {
            return 0u;
        }
        return static_cast<uint8_t>((hrSum + (hrCount / 2u)) / hrCount);
    }
};

/**
 * @brief The race.
 *
 * Lifecycle follows brief 7.3:
 *
 *   Idle -> Running <-> Paused
 *            |  |          |
 *            |  +--> Finished --> Saved
 *            +-------------------> Discarded
 */
class RaceModel
{
public:
    /// States of brief 7.3.
    enum class State : uint8_t
    {
        Idle = 0,    ///< In the menus, no race built
        Running,     ///< Clock advancing
        Paused,      ///< Clock held, for training use
        Finished,    ///< Clock stopped, not yet saved
        Saved,       ///< Written out
        Discarded,   ///< Abandoned
    };

    /// What to build when the race starts.
    struct Config
    {
        Format   format = Format::Full;  ///< Full, first half or second half
        bool     roxzone = false;        ///< Split the Roxzone in and out
        uint32_t lockoutMs = 3000u;      ///< Ignore R2 for this long after a split
    };

    // -- Template, usable without an instance ---------------------------------

    /**
     * @brief Number of segments a format produces.
     *
     * @param format  Race format.
     * @param roxzone True when Roxzone splitting is on.
     * @retval 16 or 31 for a full race, 8 or 15 for a half.
     *
     * Defined here, like label(), because the GUI needs it to say how long the
     * race will be before one exists, and the GUI binary does not link
     * RaceModel.cpp.
     */
    static constexpr uint8_t plannedCount(Format format, bool roxzone)
    {
        const uint8_t rounds =
                static_cast<uint8_t>(lastRound(format) - firstRound(format) + 1u);

        // Roxzone on is four segments a round, less the ROX_OUT that never
        // follows the final station.
        return roxzone ? static_cast<uint8_t>(rounds * 4u - 1u)
                       : static_cast<uint8_t>(rounds * 2u);
    }

    /**
     * @brief Build the segment list for a format.
     *
     * Per brief 7.2: RUN then STATION for each round, with ROX_IN and ROX_OUT
     * around the station when Roxzone splitting is on, except that no ROX_OUT
     * follows the final station -- the race ends when the last station ends.
     * A half race keeps its real round numbers, so Half B starts at RUN 5/8.
     *
     * @param format   Race format.
     * @param roxzone  True when Roxzone splitting is on.
     * @param out      Destination array.
     * @param capacity Entries available in @p out.
     * @retval Number of segments written, or 0 if @p out is null or too small.
     */
    static uint8_t buildTemplate(Format format, bool roxzone, SegmentDesc *out,
                                 uint8_t capacity);

    /**
     * @brief Format a segment label for the screen.
     *
     * Produces "RUN 3/8 \xC2\xB7 1 km", "ROXZONE IN", "SLED PULL \xC2\xB7 50 m"
     * or "ROXZONE OUT". Integer formatting only -- never %f (brief 14.4).
     *
     * @param desc Segment to describe.
     * @param buf  Caller-owned buffer, at least @c kMaxLabelLen bytes.
     * @param size Size of @p buf.
     */
    static inline void label(const SegmentDesc &desc, char *buf, size_t size)
    {
        if (buf == nullptr || size == 0u) {
            return;
        }

        switch (desc.type) {
        case SegmentType::Run:
            snprintf(buf, size, "RUN %u/%u %s %s", static_cast<unsigned>(desc.round),
                     static_cast<unsigned>(kRunCount), kLabelSep, kRunWork);
            break;

        case SegmentType::RoxIn:
            snprintf(buf, size, "%s", "ROXZONE IN");
            break;

        case SegmentType::Station:
            // stationId is 1-based; guard it because there is no MMU.
            if (desc.stationId >= 1u && desc.stationId <= kStationCount) {
                const Station &s = kStations[desc.stationId - 1u];
                snprintf(buf, size, "%s %s %s", s.name, kLabelSep, s.work);
            } else {
                snprintf(buf, size, "%s", "STATION");
            }
            break;

        case SegmentType::RoxOut:
            snprintf(buf, size, "%s", "ROXZONE OUT");
            break;

        default:
            buf[0] = '\0';
            break;
        }
    }

    /**
     * @brief Format a segment name, without its work.
     *
     * "RUN 3/8", "ROXZONE IN", "SLED PULL", "ROXZONE OUT". The one-line form of
     * label() is up to 25 characters and runs off both sides of a round 240 px
     * display, so the race face and the split toast use this and show the work
     * separately, or not at all (brief 8.2 item 4 writes the toast as
     * "SkiErg 4:12").
     *
     * Inline for the same reason as label(): the GUI binary does not link
     * RaceModel.cpp.
     *
     * @param desc Segment to describe.
     * @param buf  Caller-owned buffer, at least @c kMaxNameLen bytes.
     * @param size Size of @p buf.
     */
    static inline void name(const SegmentDesc &desc, char *buf, size_t size)
    {
        if (buf == nullptr || size == 0u) {
            return;
        }

        switch (desc.type) {
        case SegmentType::Run:
            snprintf(buf, size, "RUN %u/%u", static_cast<unsigned>(desc.round),
                     static_cast<unsigned>(kRunCount));
            break;

        case SegmentType::RoxIn:
            snprintf(buf, size, "%s", "ROXZONE IN");
            break;

        case SegmentType::Station:
            snprintf(buf, size, "%s",
                     (desc.stationId >= 1u && desc.stationId <= kStationCount)
                             ? kStations[desc.stationId - 1u].name
                             : "STATION");
            break;

        case SegmentType::RoxOut:
            snprintf(buf, size, "%s", "ROXZONE OUT");
            break;

        default:
            buf[0] = '\0';
            break;
        }
    }

    /**
     * @brief Metres this segment contributes to the FIT file.
     *
     * A run is 1 km; a station is whatever the format states; a Roxzone
     * transition and Wall Balls are nothing. Inline for the same reason label()
     * is: the GUI binary does not link RaceModel.cpp.
     *
     * @param desc Segment to measure.
     * @retval Metres, or 0 when the segment has no distance.
     */
    static inline uint16_t distanceM(const SegmentDesc &desc)
    {
        if (desc.type == SegmentType::Run) {
            return kRunDistanceM;
        }
        if (desc.type == SegmentType::Station && desc.stationId >= 1u &&
            desc.stationId <= kStationCount) {
            return kStations[desc.stationId - 1u].distanceM;
        }
        return 0u;
    }

    /**
     * @brief The work shown beside a segment name, or "" when it has none.
     *
     * @param desc Segment to describe.
     * @retval A string literal; never nullptr.
     */
    static inline const char *work(const SegmentDesc &desc)
    {
        if (desc.type == SegmentType::Run) {
            return kRunWork;
        }
        if (desc.type == SegmentType::Station && desc.stationId >= 1u &&
            desc.stationId <= kStationCount) {
            return kStations[desc.stationId - 1u].work;
        }
        return "";
    }


    // -- Events of brief 7.3 ---------------------------------------------------

    /**
     * @brief START: build the segment list and open segment 0.
     *
     * @param config Format, Roxzone mode and lockout.
     * @param nowMs  Monotonic tick to open the first segment at.
     * @retval True when the race started; false if a race is already under way.
     */
    bool start(const Config &config, uint32_t nowMs);

    /**
     * @brief SPLIT: close the open segment and open the next.
     *
     * A press inside the lockout window changes nothing at all and is recorded
     * nowhere (brief 7.5 invariant 3). Exactly at the window is accepted.
     * Closing the final segment finishes the race.
     *
     * @param pressMs Tick the button was pressed, taken in the GUI.
     * @retval True when a split was recorded, false when ignored.
     */
    bool split(uint32_t pressMs);

    /**
     * @brief UNDO_SPLIT: undo the last split.
     *
     * Reopens the previous segment with its original start time and merges the
     * time and heart-rate samples accrued since the split back into it, so
     * total time is unchanged and the merge is exact.
     *
     * @retval True when a split was undone; false at index 0 or when not running.
     */
    bool undoSplit();

    /**
     * @brief PAUSE: hold the clock.
     *
     * @param nowMs Monotonic tick.
     * @retval True when the race paused.
     */
    bool pause(uint32_t nowMs);

    /**
     * @brief RESUME: release the clock.
     *
     * @param nowMs Monotonic tick.
     * @retval True when the race resumed.
     */
    bool resume(uint32_t nowMs);

    /**
     * @brief FINISH_EARLY: stop here and mark the race incomplete.
     *
     * @param nowMs Monotonic tick to close the open segment at.
     * @retval True when the race finished.
     */
    bool finishEarly(uint32_t nowMs);

    /**
     * @brief UNDO_FINISH: take back a finishing split.
     *
     * Only a race finished by its final split can be undone; one ended early
     * cannot, because there is no press to take back (brief 7.3).
     *
     * @retval True when the finish was undone.
     */
    bool undoFinish();

    /**
     * @brief SAVE: accept the race.
     *
     * @retval True when the race moved to Saved.
     */
    bool save();

    /**
     * @brief DISCARD: abandon the race.
     *
     * @retval True when the race moved to Discarded.
     */
    bool discard();

    /**
     * @brief Attribute a heart-rate sample to the open segment.
     *
     * Samples taken while paused are dropped: they are rest, and would drag a
     * segment's average down. Validity gating (bpm > 20, trust 1 to 3) belongs
     * to the service, which owns the sensor.
     *
     * @param bpm Sample in beats per minute.
     */
    void addHeartRate(uint8_t bpm);

    // -- Queries ---------------------------------------------------------------

    /// @retval Current state.
    State state() const { return mState; }

    /// @retval True once the race is over, whether saved or not.
    bool isOver() const
    {
        return mState == State::Finished || mState == State::Saved ||
               mState == State::Discarded;
    }

    /// @retval True when the race ran to its final split rather than ending early.
    bool completed() const { return mCompleted; }

    /// @retval Segments the race was built with, 0 before it starts.
    uint8_t plannedCount() const { return mPlannedCount; }

    /// @retval Segments closed so far.
    uint8_t recordedCount() const { return mRecordedCount; }

    /// @retval Index of the open segment, equal to recordedCount() while running.
    uint8_t currentIndex() const { return mRecordedCount; }

    /// @retval The open segment, or null when no segment is open.
    const SegmentDesc *currentSegment() const;

    /// @retval The segment after the open one, or null on the last segment.
    const SegmentDesc *nextSegment() const;

    /**
     * @brief A closed segment.
     *
     * @param index Zero-based index.
     * @retval The segment, or null when @p index is out of range.
     */
    const SegmentResult *recorded(uint8_t index) const;

    /**
     * @brief Active time in the open segment.
     *
     * @param nowMs Monotonic tick.
     * @retval Milliseconds excluding pauses, 0 when no segment is open.
     */
    uint32_t currentSegmentActiveMs(uint32_t nowMs) const;

    /**
     * @brief Total active race time.
     *
     * The sum of every segment's active time, including the open one. This is
     * the timer time written to the FIT session.
     *
     * @param nowMs Monotonic tick.
     * @retval Milliseconds excluding pauses.
     */
    uint32_t totalActiveMs(uint32_t nowMs) const;

    /**
     * @brief Total wall-clock race time.
     *
     * Includes pauses (brief 7.4). Frozen once the race is over.
     *
     * @param nowMs Monotonic tick.
     * @retval Milliseconds since the race started.
     */
    uint32_t totalElapsedMs(uint32_t nowMs) const;

    /**
     * @brief Total active time across segments of one type.
     *
     * Drives the summary's runs, stations and Roxzone totals (brief 10.2).
     * Counts closed segments only.
     *
     * @param type Segment type to total.
     * @retval Milliseconds.
     */
    uint32_t totalActiveMsOfType(SegmentType type) const;

private:
    /// Close the open segment at @p atMs and bank it. Does not open the next.
    void closeCurrent(uint32_t atMs);

    /// Open the next segment at @p atMs, clearing the per-segment accumulators.
    void openSegment(uint32_t atMs);

    /// Reopen the last closed segment, merging the open one's accruals into it.
    void reopenLast();

    /// Pause milliseconds attributable to the open segment right now.
    uint32_t pausedSoFar(uint32_t nowMs) const;

    State    mState = State::Idle;
    Config   mConfig {};
    bool     mCompleted = false;

    SegmentDesc   mPlan[kMaxSegments] {};
    SegmentResult mResults[kMaxSegments] {};
    uint8_t       mPlannedCount = 0u;
    uint8_t       mRecordedCount = 0u;

    uint32_t mRaceStartMs = 0u;   ///< Tick the race began
    uint32_t mRaceEndMs = 0u;     ///< Tick the race finished; valid once over
    uint32_t mLastSplitMs = 0u;   ///< Tick of the last accepted split, for the lockout
    uint32_t mSegmentOpenMs = 0u; ///< Tick the open segment began
    uint32_t mSegmentPausedMs = 0u;  ///< Pause banked inside the open segment
    uint32_t mPauseStartMs = 0u;  ///< Tick the current pause began; valid while Paused

    uint32_t mHrSum = 0u;    ///< Heart-rate accumulators for the open segment
    uint16_t mHrCount = 0u;
    uint8_t  mHrMax = 0u;
};

}  // namespace Race

#endif  // RACE_MODEL_HPP
