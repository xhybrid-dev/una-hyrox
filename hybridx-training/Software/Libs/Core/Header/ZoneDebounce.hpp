/**
 * @file    ZoneDebounce.hpp
 * @brief   Holds a target zone steady before the zone alerts see it.
 *
 * Running on the edge of a pace band, the gauge's zone flips In / Below / In /
 * Below, and the SDK's ZoneAlerts beeps on every flip. This passes a change of
 * zone on only once the new zone has held for kDelaySec seconds in a row, so a
 * brief dip out (or a brief return in) makes no sound. The first reading after
 * Unknown or NoSignal is taken at once (the SDK already holds alerts back then),
 * and so is a fall to Unknown or NoSignal. The screen keeps the live zone.
 *
 * Pure: call tick() once per active second.
 */
#ifndef XTRAINING_ZONE_DEBOUNCE_HPP
#define XTRAINING_ZONE_DEBOUNCE_HPP

#include <cstdint>

#include "SDK/Workout/TargetGauge.hpp"

namespace XTraining {

class ZoneDebounce {
public:
    static constexpr uint32_t kDelaySec = 5;

    /// A new step: forget the held zone.
    void reset()
    {
        mHeld  = SDK::Workout::Zone::Unknown;
        mCount = 0;
    }

    /// One active second with the gauge's zone; the zone to give the alerts.
    SDK::Workout::Zone tick(SDK::Workout::Zone raw)
    {
        using Z = SDK::Workout::Zone;
        const bool rawKnown  = raw == Z::Below || raw == Z::In || raw == Z::Above;
        const bool heldKnown = mHeld == Z::Below || mHeld == Z::In || mHeld == Z::Above;
        if (!rawKnown || !heldKnown || raw == mHeld) {
            mHeld  = raw;
            mCount = 0;
            return mHeld;
        }
        if (++mCount >= kDelaySec) {
            mHeld  = raw;
            mCount = 0;
        }
        return mHeld;
    }

private:
    SDK::Workout::Zone mHeld  = SDK::Workout::Zone::Unknown;
    uint32_t           mCount = 0;   ///< Seconds the raw zone has differed from mHeld
};

} // namespace XTraining

#endif
