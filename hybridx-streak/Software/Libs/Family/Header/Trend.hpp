/**
 ******************************************************************************
 * @file    Trend.hpp
 * @brief   The smoothed trend beneath the day-to-day swings.
 *
 * The method (Jon, 12 October 2026):
 *   - **One weight per local day: the latest that day.** A later weigh-in the
 *     same day replaces an earlier one, so a slip is fixed by weighing again.
 *   - **Each day the trend moves a fixed share of the way to that day's
 *     weight**, t += (w - t) x p: an exponential moving average, the method of
 *     J. Walker's "The Hacker's Diet" (which uses p = 10%). The app uses
 *     kTrendPercent; the share can be set per Trend for comparisons. The first
 *     weigh-in starts the trend at that weight.
 *   - **Missed days** take a straight line between the weigh-ins either side,
 *     and the trend steps through each of them. A week off then neither
 *     freezes the trend nor makes it jump, and weekly weighing still works.
 *   - An entry dated before the last day seen (the clock went back) is skipped
 *     and counted; the log keeps it.
 *
 * Fixed point at 1/10 000 kg (kTrendScale units per 0.01 kg), so the result is
 * the same on the watch and the host. No heap: the last kHistoryDays days are
 * kept in a fixed ring, for the graph (W2) and the glance (W3).
 *
 * Feed it entries in log order (it is an EntryVisitor, so LogStore::scan can
 * feed it straight from the file); everything it reports includes the day in
 * hand, and a later entry that day still replaces it.
 ******************************************************************************
 */

#ifndef XWEIGHIN_TREND_HPP
#define XWEIGHIN_TREND_HPP

#include <cstdint>

#include "WeighLog.hpp"

namespace XWeighIn
{

/// Trend units per 0.01 kg: the trend is held to 1/10 000 kg.
constexpr int32_t kTrendScale  = 100;
/// The share of the gap closed each day, in percent. Jon, Gate W1: more
/// responsive than the Hacker's Diet's 10%, "just enough to smooth out
/// anomalies" (NOTES, W1).
constexpr int32_t kTrendPercent = 20;
/// Days kept for the graph and the glance.
constexpr int32_t kHistoryDays = 90;
/// The span the weekly change is taken over.
constexpr int32_t kWeekDays    = 7;

/// One day of the history.
struct DayPoint {
    int32_t day        = 0;   ///< Local day number.
    int32_t hundredths = 0;   ///< That day's weigh-in in 0.01 kg; 0 = none (a filled-in day).
    int32_t trend      = 0;   ///< The trend at the end of that day, 0.01 kg.
};

struct Summary {
    uint32_t days         = 0;      ///< Days with a weigh-in.
    int32_t  firstDay     = 0;
    int32_t  lastDay      = 0;
    int32_t  latest       = 0;      ///< The last day's weigh-in, 0.01 kg; 0 = no data.
    int32_t  trend        = 0;      ///< The trend at the last day, 0.01 kg.
    bool     hasWeekChange = false; ///< The log spans at least kWeekDays days.
    int32_t  weekChange   = 0;      ///< trend(lastDay) - trend(lastDay - 7), 0.01 kg.
    uint32_t skipped      = 0;      ///< Entries before the last day, or invalid.
};

class Trend : public EntryVisitor
{
public:
    /// @param percent  The share of the gap closed each day, 1-100 (clamped).
    explicit Trend(int32_t percent = kTrendPercent)
        : mPercent(percent < 1 ? 1 : (percent > 100 ? 100 : percent))
    {
        reset();
    }

    int32_t percent() const { return mPercent; }

    void reset();

    /// Feed one weigh-in, in log order.
    void onEntry(const Entry& e) override;

    Summary summary() const;

    /**
     * @brief   The history for one day: within the last kHistoryDays days up to
     *          and including the last day, and not before the first.
     * @return  false outside that window.
     */
    bool at(int32_t day, DayPoint& out) const;

private:
    /// Step the trend from mLastDay to @p day, ending at weight @p unitsW.
    void commit(int32_t day, int64_t unitsW);
    void record(int32_t day, int32_t hundredths, int64_t trendUnits);

    struct Snapshot {
        int32_t  lastDay;
        int64_t  lastWeightUnits;
        int64_t  trendUnits;
        uint32_t days;
    };

    int32_t  mPercent;
    bool     mStarted;
    int32_t  mFirstDay;
    int32_t  mLastDay;
    int64_t  mLastWeightUnits;   ///< The last day's weight, trend units.
    int64_t  mTrendUnits;
    uint32_t mDays;
    uint32_t mSkipped;
    /// The state before the last day was committed, so a later weigh-in the
    /// same day can replace it.
    Snapshot mBeforeLast;
    bool     mHaveBefore;

    DayPoint mRing[kHistoryDays];
};

}  // namespace XWeighIn

#endif  // XWEIGHIN_TREND_HPP
