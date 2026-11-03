/**
 ******************************************************************************
 * @file    AutoPause.hpp
 * @brief   Pauses an outdoor ride when the bike stops and resumes it when it
 *          moves off: the SDK Cycling example's detector (Examples/Apps/
 *          Cycling/Software/Libs/Header/Service.hpp, "Auto-pause"; Service.cpp,
 *          updateAutoPause), as a pure class. Header-only, no heap, integer only.
 *
 * Its rules, and the reasons the example gives for them:
 *   - Hysteresis: pause below 0.5 m/s (1.8 km/h), resume only above 0.9 m/s
 *     (3.2 km/h), so a rider hovering at one threshold cannot flap. 0.9 m/s is
 *     well below pedalling pace (a climb from a standstill must resume) and
 *     about 2.7 times the stationary GPS noise measured on a bench.
 *   - Dwell: two samples either way (three cost a second on every stop).
 *   - The dwell counts samples, not ticks: a tick with no new sample holds
 *     the evidence, neither advancing nor clearing it.
 *   - Stale speed (no valid sample for 5 s): hold the state. Pausing on a lost
 *     fix would punish a tunnel; resuming on one would punish stopping in it.
 *     The staleness starts saturated, so a ride started before the first fix
 *     is never paused on a speed that was never measured.
 *   - A manual pause wins: the detector stays quiet until the rider resumes.
 * Its thresholds are the example's provisional ones (bench noise and cycling
 * speeds, not yet a logged ride); every transition is logged by the service.
 ******************************************************************************
 */

#ifndef RIDE_AUTO_PAUSE_HPP
#define RIDE_AUTO_PAUSE_HPP

#include <cstdint>

namespace Ride {

constexpr uint32_t kAutoPauseBelowMmS  = 500;   ///< pause below, mm/s
constexpr uint32_t kAutoResumeAboveMmS = 900;   ///< resume above, mm/s
constexpr uint8_t  kAutoPauseDwell     = 2;     ///< samples either way
constexpr uint8_t  kAutoPauseStaleSec  = 5;     ///< ticks without a valid sample before the state is held

class AutoPause
{
public:
    enum class Action : uint8_t { None = 0, Pause, Resume };

    /// What the ride is doing, as the service knows it.
    enum class RideState : uint8_t { Running = 0, AutoPaused, ManuallyPaused };

    /// A new ride: no speed seen yet (stale), no evidence either way.
    void reset()
    {
        mStaleSec = kAutoPauseStaleSec;
        mBelow    = 0;
        mAbove    = 0;
    }

    /**
     * @brief   Once a second while the ride runs or is paused.
     * @param   fresh     A new, valid speed sample arrived since the last tick.
     * @param   speedMmS  That sample's speed (read only when @p fresh).
     * @param   ride      Running, paused by this detector, or paused by the rider.
     * @return  Pause or Resume when the ride should change, else None.
     */
    Action tick(bool fresh, uint32_t speedMmS, RideState ride)
    {
        if (fresh) {
            mStaleSec = 0;
        } else if (mStaleSec < kAutoPauseStaleSec) {
            ++mStaleSec;
        }
        if (mStaleSec >= kAutoPauseStaleSec) {
            mBelow = mAbove = 0;   // no trustworthy speed: hold whatever state we are in
            return Action::None;
        }
        if (!fresh) {
            return Action::None;   // a gap holds the evidence
        }
        switch (ride) {
        case RideState::Running:
            mAbove = 0;
            if (speedMmS < kAutoPauseBelowMmS) {
                if (++mBelow >= kAutoPauseDwell) {
                    mBelow = 0;
                    return Action::Pause;
                }
            } else {
                mBelow = 0;
            }
            return Action::None;
        case RideState::AutoPaused:
            mBelow = 0;
            if (speedMmS > kAutoResumeAboveMmS) {
                if (++mAbove >= kAutoPauseDwell) {
                    mAbove = 0;
                    return Action::Resume;
                }
            } else {
                mAbove = 0;
            }
            return Action::None;
        case RideState::ManuallyPaused:
        default:
            mBelow = mAbove = 0;   // the rider's pause: quiet until they resume
            return Action::None;
        }
    }

private:
    uint8_t mStaleSec = kAutoPauseStaleSec;
    uint8_t mBelow    = 0;
    uint8_t mAbove    = 0;
};

} // namespace Ride

#endif // RIDE_AUTO_PAUSE_HPP
