/**
 ******************************************************************************
 * @file    MinuteEpoch.hpp
 * @brief   One row of the Energy Probe's minute log: everything the sensors
 *          said in a minute (ten minutes when Off), summed as it arrives and
 *          written as integers (x-energy/docs/PROBE.md, "The files").
 *
 * Pure and fixed-size: no SDK, no heap. The service feeds it parsed values;
 * tests feed it made-up ones. Readings that describe a state (battery, touch,
 * the kernel's daily figures, the last arm angle and beat) carry over to the
 * next row; counts and sums start again.
 ******************************************************************************
 */

#ifndef ENERGY_PROBE_MINUTE_EPOCH_HPP
#define ENERGY_PROBE_MINUTE_EPOCH_HPP

#include <cstddef>
#include <cstdint>

namespace EnergyProbe
{

/// What the row is for, from the service.
struct RowContext {
    uint32_t    utc      = 0;        ///< the row's start, UTC seconds
    const char* local    = "";       ///< "2026-11-24T22:01", the start in local time
    uint32_t    uptimeS  = 0;        ///< seconds since boot (the service's clock / 1000)
    const char* mode     = "";       ///< modeCode()
    bool        night    = false;
};

class MinuteEpoch
{
public:
    static constexpr uint8_t kAngles     = 12;       ///< 5-second arm angles in a minute
    static constexpr uint32_t kBucketMs  = 5000;
    static constexpr int16_t kNoAngle    = INT16_MIN;
    static constexpr float   kGravity    = 9.80665f;  ///< m/s^2 per g

    MinuteEpoch() { resetAll(); }

    /// A new row: counts and sums cleared, states kept.
    void reset();
    /// Everything cleared, states included (a new service).
    void resetAll();

    // --- the service, once a message ---
    void wake() { ++mWakes; }

    // --- battery (every mode) ---
    void batteryLevel(float pct);
    void batteryCharging(bool connected, bool charging);
    void batteryMetrics(float volts, float mA, float avgMa, float mAh);

    // --- motion ---
    /// The accelerometer's unit: m/s^2 or g (ENMO in mg; the row's mean magnitude picks which,
    /// 9.8 or 1) or raw (ENMO not worked out).
    void setAccelUnitsMs2(bool ms2) { mAccelMs2 = ms2; }
    /// One sample, `offsetMs` after the row began (by the service's clock, which buckets it).
    void accel(uint32_t offsetMs, float x, float y, float z);

    // --- heart ---
    void heart(float bpm, float trust, uint8_t source, float opticalBpm);
    /// Seconds from connecting the heart rate to its first trusted reading (Heart 1 in 5).
    void heartFirstTrusted(uint32_t ms) { if (mHrFirstMs < 0) mHrFirstMs = static_cast<int32_t>(ms); }
    void beat(uint64_t timestampUs);

    // --- body and events ---
    void temperature(float celsius);
    void touch(bool on);
    void motionDetect(uint32_t id);
    void activity(uint32_t id, uint32_t confidence);
    void wrist() { ++mWristN; }

    // --- the kernel's daily figures ---
    void dailyHeart(float ahr, float rhr);
    void dailySteps(uint32_t steps) { mSteps = static_cast<int64_t>(steps); }
    void dailyActive(uint32_t v) { mActive = static_cast<int64_t>(v); }
    void dailyFloors(int32_t up) { mFloors = up; }

    // --- the row ---
    static const char* header();          ///< the CSV header line, with its newline
    static uint8_t     columns();         ///< how many columns header() names
    /// The row for `ctx`, with its newline. Returns the length, 0 if `cap` is too small.
    size_t format(char* out, size_t cap, const RowContext& ctx) const;

    // --- for the screen and the night's sums ---
    bool    hasBattery() const { return mPctX100 >= 0; }
    int32_t batteryPctX100() const { return mPctX100; }
    int32_t batteryMahX10() const { return mMahX10; }
    int32_t batteryAvgMaX100() const { return mAvgMaX100; }
    bool    hasAvgCurrent() const { return mHasMetrics; }
    int8_t  charging() const { return mCharging; }   ///< -1 unknown, 0 no, 1 yes
    int8_t  usb() const { return mUsb; }
    uint32_t heartTrusted() const { return mHrOk; }
    int8_t  touchState() const { return mTouch; }
    uint8_t lastBpm() const { return mLastBpm; }
    uint32_t accelSamples() const { return mAccN; }
    uint32_t beats() const { return mBeatN; }
    /// A minute is worn if the touch sensor said so or a trusted heart rate arrived.
    bool    worn() const { return mTouch == 1 || mHrOk > 0; }

private:
    // states
    int32_t  mPctX100      = -1;
    int8_t   mCharging     = -1;
    int8_t   mUsb          = -1;
    int32_t  mMv           = -1;
    int32_t  mMaX100       = 0;
    int32_t  mAvgMaX100    = 0;
    int32_t  mMahX10       = -1;
    bool     mHasMetrics   = false;
    bool     mAccelMs2     = true;
    int16_t  mLastAngle    = kNoAngle;   ///< the last bucket's angle, for the change across rows
    int8_t   mTouch        = -1;
    int32_t  mAhrX10       = -1;
    int32_t  mRhrX10       = -1;
    int64_t  mSteps        = -1;
    int64_t  mActive       = -1;
    int32_t  mFloors       = INT32_MIN;
    uint32_t mArId         = 0xFFFFFFFFu;
    uint32_t mArConf       = 0;
    uint64_t mLastBeatUs   = 0;
    int32_t  mPrevRrMs     = -1;
    uint8_t  mLastBpm      = 0;
    uint8_t  mHrSource     = 0xFF;

    // this row
    uint32_t mBattN        = 0;
    uint32_t mWakes        = 0;
    uint32_t mAccN         = 0;
    double   mMagSum       = 0;
    double   mEnmoSum      = 0;   ///< mg, if the samples are in m/s^2
    double   mEnmoSumG     = 0;   ///< mg, if they are in g (an older SDK document said g)
    float    mBx[kAngles]  {};
    float    mBy[kAngles]  {};
    float    mBz[kAngles]  {};
    uint16_t mBn[kAngles]  {};
    uint32_t mHrN          = 0;
    uint32_t mHrOk         = 0;
    uint32_t mHrMin        = 0;
    uint32_t mHrMax        = 0;
    double   mHrSum        = 0;
    double   mTrustSum     = 0;
    uint32_t mOptZero      = 0;
    int32_t  mHrFirstMs    = -1;
    double   mTempSum      = 0;
    uint32_t mTempN        = 0;
    uint32_t mTouchChanges = 0;
    uint32_t mMd[3]        {};
    uint32_t mArN          = 0;
    uint32_t mWristN       = 0;
    uint32_t mBeatN        = 0;
    uint32_t mRrN          = 0;
    double   mRrSum        = 0;
    double   mRrDiffSq     = 0;
    uint32_t mRrDiffN      = 0;
    uint32_t mRrBad        = 0;
};

/// The Race gate for a trusted heart rate (hybridx-race/docs/NOTES.md, HEART_RATE_EX).
inline bool trustedHeart(float bpm, float trust)
{
    return bpm > 20.0f && trust >= 1.0f && trust <= 3.0f;
}

/// The arm angle in tenths of a degree from a mean gravity vector (van Hees 2015:
/// atan(z / sqrt(x^2 + y^2))); kNoAngle if the vector is zero.
int16_t armAngleX10(float x, float y, float z);

} // namespace EnergyProbe

#endif // ENERGY_PROBE_MINUTE_EPOCH_HPP
