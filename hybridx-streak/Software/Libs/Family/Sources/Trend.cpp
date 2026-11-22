/**
 ******************************************************************************
 * @file    Trend.cpp
 * @brief   The smoothed trend (see the header).
 ******************************************************************************
 */

#include "Trend.hpp"

namespace XWeighIn
{

namespace
{

/// Integer division rounding half away from zero.
int64_t divRound(int64_t num, int64_t den)
{
    return num >= 0 ? (num + den / 2) / den : -((-num + den / 2) / den);
}

int32_t toHundredths(int64_t units)
{
    return static_cast<int32_t>(divRound(units, kTrendScale));
}

int32_t slot(int32_t day)
{
    const int32_t s = day % kHistoryDays;
    return s < 0 ? s + kHistoryDays : s;
}

}  // namespace

void Trend::reset()
{
    mStarted         = false;
    mFirstDay        = 0;
    mLastDay         = 0;
    mLastWeightUnits = 0;
    mTrendUnits      = 0;
    mDays            = 0;
    mSkipped         = 0;
    mBeforeLast      = Snapshot {0, 0, 0, 0};
    mHaveBefore      = false;
    for (DayPoint& p : mRing) {
        p = DayPoint {};
    }
}

void Trend::record(int32_t day, int32_t hundredths, int64_t trendUnits)
{
    DayPoint& p  = mRing[slot(day)];
    p.day        = day;
    p.hundredths = hundredths;
    p.trend      = toHundredths(trendUnits);
}

void Trend::commit(int32_t day, int64_t unitsW)
{
    // Remember where we were, so a later weigh-in the same day can replace this one.
    mBeforeLast = Snapshot {mLastDay, mLastWeightUnits, mTrendUnits, mDays};
    mHaveBefore = true;

    const int64_t gap = static_cast<int64_t>(day) - mLastDay;   // >= 1
    for (int64_t k = 1; k <= gap; ++k) {
        // The day's weight: the weigh-in on the last step, a straight line
        // between the two weigh-ins before it.
        const int64_t w = (k == gap) ? unitsW
                                     : mLastWeightUnits + divRound((unitsW - mLastWeightUnits) * k, gap);
        mTrendUnits += divRound((w - mTrendUnits) * mPercent, 100);
        const int32_t d = mLastDay + static_cast<int32_t>(k);
        record(d, (k == gap) ? static_cast<int32_t>(unitsW / kTrendScale) : 0, mTrendUnits);
    }
    mLastDay         = day;
    mLastWeightUnits = unitsW;
    ++mDays;
}

void Trend::onEntry(const Entry& e)
{
    if (!isValidEntry(e)) {
        ++mSkipped;
        return;
    }
    const int64_t unitsW = static_cast<int64_t>(e.hundredths) * kTrendScale;

    if (!mStarted) {
        mStarted         = true;
        mFirstDay        = e.day;
        mLastDay         = e.day;
        mLastWeightUnits = unitsW;
        mTrendUnits      = unitsW;
        mDays            = 1;
        mHaveBefore      = false;
        record(e.day, e.hundredths, mTrendUnits);
        return;
    }

    if (e.day < mLastDay) {
        ++mSkipped;
        return;
    }

    if (e.day == mLastDay) {
        // The latest weigh-in of the day counts: undo the day and redo it.
        if (!mHaveBefore) {
            // The first day of the log: the trend simply starts again here.
            mLastWeightUnits = unitsW;
            mTrendUnits      = unitsW;
            record(e.day, e.hundredths, mTrendUnits);
            return;
        }
        mLastDay         = mBeforeLast.lastDay;
        mLastWeightUnits = mBeforeLast.lastWeightUnits;
        mTrendUnits      = mBeforeLast.trendUnits;
        mDays            = mBeforeLast.days;
    }
    commit(e.day, unitsW);
}

Summary Trend::summary() const
{
    Summary s;
    s.skipped = mSkipped;
    if (!mStarted) {
        return s;
    }
    s.days     = mDays;
    s.firstDay = mFirstDay;
    s.lastDay  = mLastDay;
    s.latest   = static_cast<int32_t>(mLastWeightUnits / kTrendScale);
    s.trend    = toHundredths(mTrendUnits);

    DayPoint weekAgo;
    if (mLastDay - mFirstDay >= kWeekDays && at(mLastDay - kWeekDays, weekAgo)) {
        s.hasWeekChange = true;
        s.weekChange    = s.trend - weekAgo.trend;
    }
    return s;
}

bool Trend::at(int32_t day, DayPoint& out) const
{
    if (!mStarted || day > mLastDay || day < mFirstDay || day <= mLastDay - kHistoryDays) {
        return false;
    }
    const DayPoint& p = mRing[slot(day)];
    if (p.day != day) {
        return false;
    }
    out = p;
    return true;
}

}  // namespace XWeighIn
