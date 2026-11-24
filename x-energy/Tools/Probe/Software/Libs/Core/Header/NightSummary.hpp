/**
 ******************************************************************************
 * @file    NightSummary.hpp
 * @brief   A night's battery drain and coverage, for nights.csv and the
 *          Nights page (x-energy/docs/PROBE.md, "The battery evidence").
 *
 * Pure, integers only. The tracker opens at the first row of a night, adds
 * each row, and closes on the first row after it. Its state is saved with the
 * probe's state, so a reboot mid-night carries on (flagged).
 ******************************************************************************
 */

#ifndef ENERGY_PROBE_NIGHT_SUMMARY_HPP
#define ENERGY_PROBE_NIGHT_SUMMARY_HPP

#include <cstddef>
#include <cstdint>

namespace EnergyProbe
{

enum NightFlag : uint8_t {
    kFlagCharged   = 1u << 0,   ///< on the charger at some point: the drain is not a drain
    kFlagRestarted = 1u << 1,   ///< the service restarted during the night (a reboot, USB)
    kFlagPartial   = 1u << 2,   ///< began more than 10 minutes after the window opened
    kFlagNoBattery = 1u << 3,   ///< no battery reading at the start or end
    kFlagModeChanged = 1u << 4, ///< the mode was changed during the night (the Tonight page)
};

/// The running sums of the night in progress (saved in state.txt).
struct NightOpen {
    bool     open        = false;
    int32_t  day         = 0;     ///< the night's day number (its evening's date)
    uint8_t  mode        = 0;     ///< Mode
    uint8_t  flags       = 0;
    uint32_t startUtc    = 0;
    int32_t  startPct    = -1;    ///< battery % x100 at the start; -1 unknown
    int32_t  startMah    = -1;    ///< remaining mAh x10 at the start; -1 unknown
    uint32_t rows        = 0;
    uint32_t wornMin     = 0;
    uint32_t hrMin       = 0;
    int64_t  gaugeSum    = 0;     ///< sum of the fuel gauge's average mA x100, one per row
    uint32_t gaugeN      = 0;
    uint32_t lastUtc     = 0;     ///< the last row's values, to close a night a reboot cut short
    int32_t  lastPct     = -1;
    int32_t  lastMah     = -1;
};

/// A closed night.
struct NightResult {
    int32_t  day       = 0;
    uint8_t  mode      = 0;
    uint8_t  flags     = 0;
    uint32_t startUtc  = 0;
    uint32_t endUtc    = 0;
    int32_t  startPct  = -1;     ///< x100
    int32_t  endPct    = -1;     ///< x100
    int32_t  hoursX100 = 0;
    int32_t  drainX100 = INT32_MIN;   ///< % an hour x100 (0.42 %/h = 42); INT32_MIN unknown
    int32_t  mahDropX10 = INT32_MIN;  ///< INT32_MIN unknown
    int32_t  capMaX100  = INT32_MIN;  ///< mean current from the mAh drop, x100
    int32_t  gaugeMaX100 = INT32_MIN; ///< mean of the gauge's average current, x100
    uint32_t rows      = 0;
    uint32_t wornMin   = 0;
    uint32_t hrMin     = 0;
};

/// What the Nights page shows of a night (also kept in state.txt). Plain data.
struct NightBrief {
    int32_t day         = 0;
    int32_t drainX100   = INT32_MIN;
    int32_t capMaX100   = INT32_MIN;
    int32_t gaugeMaX100 = INT32_MIN;
    int32_t hoursX100   = 0;
    uint8_t mode        = 0;
    uint8_t flags       = 0;
    uint8_t pad[2]      = {};
};

inline NightBrief briefOf(const NightResult& r)
{
    NightBrief b;
    b.day = r.day;
    b.drainX100 = r.drainX100;
    b.capMaX100 = r.capMaX100;
    b.gaugeMaX100 = r.gaugeMaX100;
    b.hoursX100 = r.hoursX100;
    b.mode = r.mode;
    b.flags = r.flags;
    return b;
}

class NightTracker
{
public:
    /// Open a night at its first row. `minutesLate` is how far into the window it began.
    void open(int32_t day, uint8_t mode, uint32_t utc, int32_t pctX100, int32_t mahX10, uint32_t minutesLate);

    /// A row of `minutes` minutes ended at `utc`.
    void addRow(uint32_t utc, uint32_t minutes, bool worn, bool heart, int8_t charging, int32_t pctX100,
                int32_t mahX10, bool hasGauge, int32_t gaugeMaX100);

    /// Close it at `utc` with the last battery values.
    NightResult close(uint32_t utc, int32_t pctX100, int32_t mahX10);
    /// Close it with the last row's values (a reboot ended it unseen).
    NightResult closeAtLastRow();

    void markRestarted() { mOpen.flags |= kFlagRestarted; }
    /// The wearer changed tonight's mode while it was on: the night takes the new mode, flagged.
    void changeMode(uint8_t mode)
    {
        if (mOpen.open && mOpen.mode != mode) {
            mOpen.mode = mode;
            mOpen.flags |= kFlagModeChanged;
        }
    }
    bool isOpen() const { return mOpen.open; }
    const NightOpen& state() const { return mOpen; }
    void restore(const NightOpen& s) { mOpen = s; }

private:
    NightOpen mOpen;
};

/// The header of nights.csv, with its newline.
const char* nightHeader();
/// A night as a nights.csv row, with its newline; 0 if `cap` is too small.
size_t formatNight(char* out, size_t cap, const NightResult& n, const char* modeCode, uint32_t yyyymmdd);

/// Drain in % an hour x100 from two % x100 readings `seconds` apart; INT32_MIN if unknown.
int32_t drainPerHourX100(int32_t startPct, int32_t endPct, uint32_t seconds);

} // namespace EnergyProbe

#endif // ENERGY_PROBE_NIGHT_SUMMARY_HPP
