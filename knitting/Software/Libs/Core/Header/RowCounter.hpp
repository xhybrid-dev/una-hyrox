/**
 ******************************************************************************
 * @file    RowCounter.hpp
 * @brief   The knitting counter: rows in the current set, the set number and
 *          the total rows. Pure C++, header-only, no kernel and no clock.
 *
 * Header-only because the GUI process links none of the Libs sources.
 * Counters saturate rather than wrap, so a stuck button can never show a
 * smaller number than it did. The limits keep every number within the
 * display's digits: three for rows and sets, five for the total.
 ******************************************************************************
 */

#ifndef ROW_COUNTER_HPP
#define ROW_COUNTER_HPP

#include <cstdint>

namespace Knitting {

constexpr uint32_t kMaxRows  = 999;     ///< Rows in one set.
constexpr uint32_t kMaxSets  = 999;
constexpr uint32_t kMaxTotal = 99999;

/// Plain data, so it can travel in a fixed-size message and be copied freely.
struct CounterState {
    uint32_t rows  = 0;     ///< Rows knitted in the current set.
    uint32_t set   = 1;     ///< The current set, from 1.
    uint32_t total = 0;     ///< Rows knitted in all sets.
};

class RowCounter
{
public:
    RowCounter() = default;

    explicit RowCounter(const CounterState& state) : mState(clamp(state)) {}

    const CounterState& state() const { return mState; }

    /// One row done. False when a limit stopped it.
    bool countRow()
    {
        if (mState.rows >= kMaxRows || mState.total >= kMaxTotal) {
            return false;
        }
        ++mState.rows;
        ++mState.total;
        return true;
    }

    /**
     * @brief   Start the next set. False when the current set is empty (a
     *          double press must not leave a run of empty sets) or at the
     *          last set.
     */
    bool nextSet()
    {
        if (mState.rows == 0 || mState.set >= kMaxSets) {
            return false;
        }
        ++mState.set;
        mState.rows = 0;
        return true;
    }

    /// Back to the start: no rows, set 1. False when it already was, so
    /// nothing changed and nothing needs saving.
    bool reset()
    {
        const CounterState fresh;
        const bool changed = mState.rows != fresh.rows || mState.set != fresh.set
                          || mState.total != fresh.total;
        mState = fresh;
        return changed;
    }

private:
    /// A state read from a file is not trusted: pull it inside the limits.
    static CounterState clamp(CounterState s)
    {
        if (s.rows > kMaxRows)   { s.rows = kMaxRows; }
        if (s.set < 1)           { s.set = 1; }
        if (s.set > kMaxSets)    { s.set = kMaxSets; }
        if (s.total > kMaxTotal) { s.total = kMaxTotal; }
        return s;
    }

    CounterState mState;
};

} // namespace Knitting

#endif // ROW_COUNTER_HPP
