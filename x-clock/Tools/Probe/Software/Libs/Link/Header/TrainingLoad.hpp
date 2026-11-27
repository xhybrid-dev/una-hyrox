/**
 ******************************************************************************
 * @file    TrainingLoad.hpp
 * @brief   A session's training load from heart rate: Edwards' TRIMP. Each
 *          minute in heart-rate zone 1 to 5 counts 1 to 5 points (Edwards,
 *          "The Heart Rate Monitor Book", 1993). Pure C++, header-only, no
 *          heap, integer only.
 *
 * Zones are the watch's own (RequestSystemSettings' heartRateTh, as every
 * HybridX app reads them): above threshold i is zone i + 1, capped at 5, and
 * at or below the first threshold, or with no heart rate, is zone 0, which
 * scores nothing. The same rule as Workout's and Ride's HrZones::zoneOf and
 * Run's RunStats.
 *
 * An app that already counts time in each zone (Run, Workout, Ride) scores the
 * session with edwardsLoad(); one that does not (Race, Trail) feeds a
 * LoadMeter once a second while the activity runs. The load goes in the
 * activity summary (ActivityLink.hpp, Entry::load) for HybridX Streak, which
 * builds fitness, fatigue and form from it.
 *
 * Canonical source: hybridx-link/. Each app keeps a byte-identical copy in its
 * Software/Libs/Link/; Utilities/check-link-copies.sh keeps them so.
 ******************************************************************************
 */

#ifndef HYBRIDX_LINK_TRAINING_LOAD_HPP
#define HYBRIDX_LINK_TRAINING_LOAD_HPP

#include <cstdint>

namespace HybridXLink
{

/// Zone 0 (below zone 1, or no heart rate) and zones 1 to 5.
inline constexpr uint8_t kLoadZones = 6;

/// The highest load a summary holds (48 hours in zone 5 is 14400).
inline constexpr uint16_t kMaxLoad = 65535;

/// The zone of @p bpm on @p n thresholds (bpm 0: none, zone 0).
constexpr uint8_t loadZoneOf(uint16_t bpm, const uint8_t* th, uint8_t n)
{
    if (bpm == 0 || th == nullptr) {
        return 0;
    }
    uint8_t zone = 0;
    for (uint8_t i = 0; i < n; ++i) {
        if (bpm > th[i]) {
            zone = static_cast<uint8_t>(i + 1);
        }
    }
    return zone > 5 ? 5 : zone;
}

namespace detail
{
constexpr uint16_t capLoad(uint64_t v)
{
    return v > kMaxLoad ? kMaxLoad : static_cast<uint16_t>(v);
}
}  // namespace detail

/// The load of a session from the seconds it spent in each zone ([0] is below
/// zone 1 and counts nothing): the sum of zone x minutes, rounded.
constexpr uint16_t edwardsLoad(const uint32_t zoneSec[kLoadZones])
{
    uint64_t pointSec = 0;
    for (uint8_t z = 1; z < kLoadZones; ++z) {
        pointSec += static_cast<uint64_t>(z) * zoneSec[z];
    }
    return detail::capLoad((pointSec + 30u) / 60u);
}

/// The same from milliseconds in each zone (Workout's and Ride's models).
constexpr uint16_t edwardsLoadMs(const uint32_t zoneMs[kLoadZones])
{
    uint64_t pointMs = 0;
    for (uint8_t z = 1; z < kLoadZones; ++z) {
        pointMs += static_cast<uint64_t>(z) * zoneMs[z];
    }
    return detail::capLoad((pointMs + 30000u) / 60000u);
}

/// Counts the seconds in each zone for an app that keeps no zones of its own.
/// Call addSecond() once a second while the activity runs (not while paused),
/// with the heart rate it would record (0 when there is none or it is not
/// trusted).
class LoadMeter
{
public:
    /// The watch's thresholds (at most kLoadZones are used). Also clears the count.
    void setZones(const uint8_t* th, uint8_t n)
    {
        mCount = 0;
        for (uint8_t i = 0; th != nullptr && i < n && i < kLoadZones; ++i) {
            mTh[i] = th[i];
            mCount = static_cast<uint8_t>(i + 1);
        }
        reset();
    }

    /// A new activity: no seconds counted.
    void reset()
    {
        for (uint8_t z = 0; z < kLoadZones; ++z) {
            mSec[z] = 0;
        }
    }

    void addSecond(uint16_t bpm)
    {
        const uint8_t z = loadZoneOf(bpm, mTh, mCount);
        if (mSec[z] < UINT32_MAX) {
            ++mSec[z];
        }
    }

    uint16_t load() const { return edwardsLoad(mSec); }
    uint32_t secondsIn(uint8_t zone) const { return zone < kLoadZones ? mSec[zone] : 0u; }

private:
    uint8_t  mTh[kLoadZones] = {};
    uint8_t  mCount = 0;
    uint32_t mSec[kLoadZones] = {};
};

}  // namespace HybridXLink

#endif  // HYBRIDX_LINK_TRAINING_LOAD_HPP
