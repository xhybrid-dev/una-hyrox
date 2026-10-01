/**
 ******************************************************************************
 * @file    RecordSpool.hpp
 * @brief   Holds the open segment's per-second FIT records until the segment's
 *          distance is known, then writes them with a distance that ramps.
 ******************************************************************************
 * Why this exists: Strava and Garmin build lap pace, moving time and splits
 * from the distance in the per-second records, not from the lap totals. A race
 * has no GPS, so the only distance is what the format states for a segment, and
 * the watch only learns that a segment is over when the athlete presses the
 * button. Writing the distance live would make it jump by a kilometre in one
 * second (a 1000 m/s "speed", and Strava would see almost no moving time at
 * all). Instead the records of the open segment wait here, and when the split
 * comes they are written with the distance spread evenly across them.
 *
 * Pure C++: no SDK, no heap, no floats, no clock. Host-tested.
 *
 * Distance never goes backwards. A split can be undone, which shrinks the sum
 * of completed segments, but the records already written cannot be unwritten:
 * the spool keeps the highest distance it has written and ramps from there, so
 * an undone-and-redone segment is not counted twice.
 ******************************************************************************
 */

#ifndef RECORD_SPOOL_HPP
#define RECORD_SPOOL_HPP

#include <cstdint>

namespace Race {

/**
 * @tparam Rec       Trivially copyable record type.
 * @tparam Capacity  Records held. A segment longer than this writes its oldest
 *                   records with a flat distance rather than losing them.
 *
 * A sink is a callable taking (const Rec &, uint32_t distanceCm).
 */
template <typename Rec, uint16_t Capacity>
class RecordSpool {
    static_assert(Capacity > 0u, "RecordSpool needs room for at least one record");

public:
    uint16_t size() const { return mCount; }

    /// Highest distance written so far, in centimetres (the FIT scale).
    uint32_t writtenCm() const { return mWrittenCm; }

    /**
     * @brief Keep a record for the open segment.
     * When the spool is full the oldest record goes to the sink first, at the
     * distance already written, so nothing is dropped.
     */
    template <typename Sink>
    void add(const Rec &rec, Sink &&sink)
    {
        if (mCount == Capacity) {
            sink(mBuf[mHead], mWrittenCm);
            mHead = next(mHead);
            --mCount;
        }
        mBuf[(mHead + mCount) % Capacity] = rec;
        ++mCount;
    }

    /**
     * @brief The segment is over: write its records, ramping the distance up to
     *        @p targetCm, the total of every completed segment.
     *
     * The last record carries exactly @p targetCm. A target below what has
     * already been written (after an undo) is raised to it, so the stream stays
     * monotonic.
     */
    template <typename Sink>
    void close(uint32_t targetCm, Sink &&sink)
    {
        if (targetCm < mWrittenCm) {
            targetCm = mWrittenCm;
        }
        const uint32_t fromCm = mWrittenCm;
        const uint64_t span = static_cast<uint64_t>(targetCm) - fromCm;
        const uint32_t n = mCount;

        for (uint32_t i = 0u; i < n; ++i) {
            const uint32_t cm = fromCm + static_cast<uint32_t>((span * (i + 1u)) / n);
            sink(mBuf[(mHead + i) % Capacity], cm);
        }

        mHead = 0u;
        mCount = 0u;
        mWrittenCm = targetCm;
    }

    /// Forget everything, for a new race.
    void reset()
    {
        mHead = 0u;
        mCount = 0u;
        mWrittenCm = 0u;
    }

private:
    static uint16_t next(uint16_t i) { return static_cast<uint16_t>((i + 1u) % Capacity); }

    Rec      mBuf[Capacity] = {};
    uint16_t mHead = 0u;
    uint16_t mCount = 0u;
    uint32_t mWrittenCm = 0u;
};

}  // namespace Race

#endif  // RECORD_SPOOL_HPP
