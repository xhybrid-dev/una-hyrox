/**
 ******************************************************************************
 * @file    SecondGate.hpp
 * @brief   Lets one call through for each new second of the clock. Pure C++,
 *          header-only.
 *
 * The service polls every 200 ms or so and wants exactly one record for each
 * UTC second. A timer that waits "a second since the last one" drifts a little
 * slow, and about every twentieth second it is skipped: Jon's first session
 * (22 October 2026) had 2,104 records for 2,205 s, a 2 s gap every 20 s or so.
 * Keying the record on the second itself leaves no drift to accumulate.
 *
 * A stall (the loop busy for several seconds) lets one call through, not one
 * for each second missed: the file never gets two records with one time.
 ******************************************************************************
 */

#ifndef RIDE_SECOND_GATE_HPP
#define RIDE_SECOND_GATE_HPP

#include <cstdint>

namespace Ride {

class SecondGate {
public:
    /// True the first time @p utcSec is later than any seen before.
    bool take(int64_t utcSec)
    {
        if (mHave && utcSec <= mLast) {
            return false;
        }
        mHave = true;
        mLast = utcSec;
        return true;
    }

    /// Forget everything: the next call goes through.
    void reset() { mHave = false; }

private:
    bool    mHave = false;
    int64_t mLast = 0;
};

} // namespace Ride

#endif // RIDE_SECOND_GATE_HPP
