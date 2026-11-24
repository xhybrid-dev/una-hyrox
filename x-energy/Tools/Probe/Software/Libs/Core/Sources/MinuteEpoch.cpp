/**
 ******************************************************************************
 * @file    MinuteEpoch.cpp
 * @brief   One row of the minute log (see the header).
 ******************************************************************************
 */

#include "MinuteEpoch.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "Fixed.hpp"

namespace EnergyProbe
{

namespace
{

constexpr uint32_t kRrMinMs = 300;    ///< 200 bpm
constexpr uint32_t kRrMaxMs = 2000;   ///< 30 bpm

/// A bounded text builder: once full, it stays failed.
class Out
{
public:
    Out(char* buf, size_t cap) : mBuf(buf), mCap(cap) {}

    void text(const char* s)
    {
        if (!mOk) {
            return;
        }
        const size_t n = std::strlen(s);
        if (mLen + n + 1 > mCap) {
            mOk = false;
            return;
        }
        std::memcpy(mBuf + mLen, s, n);
        mLen += n;
        mBuf[mLen] = '\0';
    }
    void comma() { text(","); }
    void i(int64_t v)
    {
        char t[24];
        // Two 32-bit halves at most: the watch's printf may not do 64-bit integers.
        if (v > 2147483647LL || v < -2147483647LL) {
            std::snprintf(t, sizeof(t), "%s", "");
        } else {
            std::snprintf(t, sizeof(t), "%ld", static_cast<long>(v));
        }
        text(t);
    }
    /// An integer, or an empty field when `has` is false.
    void maybe(bool has, int64_t v)
    {
        if (has) {
            i(v);
        }
    }
    bool   ok() const { return mOk; }
    size_t len() const { return mLen; }

private:
    char*  mBuf;
    size_t mCap;
    size_t mLen = 0;
    bool   mOk  = true;
};

int32_t roundToInt(double v)
{
    return static_cast<int32_t>(v >= 0 ? v + 0.5 : v - 0.5);
}

} // namespace

int16_t armAngleX10(float x, float y, float z)
{
    const double h = std::sqrt(static_cast<double>(x) * x + static_cast<double>(y) * y);
    if (h == 0.0 && z == 0.0f) {
        return MinuteEpoch::kNoAngle;
    }
    const double deg = std::atan2(static_cast<double>(z), h) * 180.0 / 3.14159265358979323846;
    return static_cast<int16_t>(roundToInt(deg * 10.0));
}

void MinuteEpoch::reset()
{
    // Keep the row's last arm angle, so the next row's change starts from it.
    for (int b = kAngles - 1; b >= 0; --b) {
        if (mBn[b] > 0) {
            const int16_t a = armAngleX10(mBx[b] / mBn[b], mBy[b] / mBn[b], mBz[b] / mBn[b]);
            if (a != kNoAngle) {
                mLastAngle = a;
            }
            break;
        }
    }
    mBattN = 0;
    mWakes = 0;
    mAccN = 0;
    mMagSum = 0;
    mEnmoSum = 0;
    for (uint8_t b = 0; b < kAngles; ++b) {
        mBx[b] = mBy[b] = mBz[b] = 0.0f;
        mBn[b] = 0;
    }
    mHrN = mHrOk = mHrMin = mHrMax = 0;
    mHrSum = mTrustSum = 0;
    mOptZero = 0;
    mHrFirstMs = -1;
    mTempSum = 0;
    mTempN = 0;
    mTouchChanges = 0;
    mMd[0] = mMd[1] = mMd[2] = 0;
    mArN = 0;
    mWristN = 0;
    mBeatN = mRrN = mRrDiffN = mRrBad = 0;
    mRrSum = mRrDiffSq = 0;
}

void MinuteEpoch::resetAll()
{
    reset();
    mPctX100 = -1;
    mCharging = mUsb = -1;
    mMv = -1;
    mMaX100 = mAvgMaX100 = 0;
    mMahX10 = -1;
    mHasMetrics = false;
    mLastAngle = kNoAngle;
    mTouch = -1;
    mAhrX10 = mRhrX10 = -1;
    mSteps = mActive = -1;
    mFloors = INT32_MIN;
    mArId = 0xFFFFFFFFu;
    mArConf = 0;
    mLastBeatUs = 0;
    mPrevRrMs = -1;
    mLastBpm = 0;
    mHrSource = 0xFF;
}

void MinuteEpoch::batteryLevel(float pct)
{
    const int64_t v = scaled(pct, 100);
    if (v != kNoValue && v >= 0 && v <= 10000) {
        mPctX100 = static_cast<int32_t>(v);
        ++mBattN;
    }
}

void MinuteEpoch::batteryCharging(bool connected, bool charging)
{
    mUsb = connected ? 1 : 0;
    mCharging = charging ? 1 : 0;
}

void MinuteEpoch::batteryMetrics(float volts, float mA, float avgMa, float mAh)
{
    const int64_t mv  = scaled(volts, 1000);
    const int64_t ma  = scaled(mA, 100);
    const int64_t avg = scaled(avgMa, 100);
    const int64_t cap = scaled(mAh, 10);
    if (mv == kNoValue || ma == kNoValue || avg == kNoValue || cap == kNoValue) {
        return;
    }
    if (mv < 0 || mv > 10000 || ma < -10000000 || ma > 10000000 || avg < -10000000 || avg > 10000000 ||
        cap < 0 || cap > 1000000) {
        return;
    }
    mMv = static_cast<int32_t>(mv);
    mMaX100 = static_cast<int32_t>(ma);
    mAvgMaX100 = static_cast<int32_t>(avg);
    mMahX10 = static_cast<int32_t>(cap);
    mHasMetrics = true;
}

void MinuteEpoch::accel(uint32_t offsetMs, float x, float y, float z)
{
    if (!(x == x) || !(y == y) || !(z == z)) {
        return;
    }
    uint32_t b = offsetMs / kBucketMs;
    if (b >= kAngles) {
        b = kAngles - 1;
    }
    mBx[b] += x;
    mBy[b] += y;
    mBz[b] += z;
    if (mBn[b] < 0xFFFF) {
        ++mBn[b];
    }
    const double mag = std::sqrt(static_cast<double>(x) * x + static_cast<double>(y) * y +
                                 static_cast<double>(z) * z);
    mMagSum += mag;
    if (mAccelMs2) {
        const double enmo = mag / kGravity - 1.0;
        mEnmoSum += enmo > 0 ? enmo * 1000.0 : 0.0;
    }
    ++mAccN;
}

void MinuteEpoch::heart(float bpm, float trust, uint8_t source, float opticalBpm)
{
    ++mHrN;
    mHrSource = source;
    if (!(opticalBpm > 0.0f)) {
        ++mOptZero;
    }
    if (trust == trust) {
        mTrustSum += trust;
    }
    if (!trustedHeart(bpm, trust) || bpm > 250.0f) {
        return;
    }
    const uint32_t b = static_cast<uint32_t>(bpm + 0.5f);
    if (mHrOk == 0 || b < mHrMin) {
        mHrMin = b;
    }
    if (b > mHrMax) {
        mHrMax = b;
    }
    mHrSum += bpm;
    ++mHrOk;
    mLastBpm = static_cast<uint8_t>(b);
}

void MinuteEpoch::beat(uint64_t timestampUs)
{
    ++mBeatN;
    if (mLastBeatUs != 0 && timestampUs > mLastBeatUs) {
        const uint64_t rr = (timestampUs - mLastBeatUs) / 1000u;
        if (rr >= kRrMinMs && rr <= kRrMaxMs) {
            const int32_t r = static_cast<int32_t>(rr);
            ++mRrN;
            mRrSum += r;
            if (mPrevRrMs >= 0) {
                const double d = static_cast<double>(r - mPrevRrMs);
                mRrDiffSq += d * d;
                ++mRrDiffN;
            }
            mPrevRrMs = r;
        } else {
            ++mRrBad;
            mPrevRrMs = -1;   // a gap or an artefact: the next difference starts again
        }
    } else if (mLastBeatUs != 0) {
        ++mRrBad;   // out of order or repeated
        mPrevRrMs = -1;
    }
    mLastBeatUs = timestampUs;
}

void MinuteEpoch::temperature(float celsius)
{
    if (celsius == celsius && celsius > -100.0f && celsius < 200.0f) {
        mTempSum += celsius;
        ++mTempN;
    }
}

void MinuteEpoch::touch(bool on)
{
    const int8_t v = on ? 1 : 0;
    if (mTouch >= 0 && mTouch != v) {
        ++mTouchChanges;
    }
    mTouch = v;
}

void MinuteEpoch::motionDetect(uint32_t id)
{
    if (id < 3) {
        ++mMd[id];
    }
}

void MinuteEpoch::activity(uint32_t id, uint32_t confidence)
{
    mArId = id;
    mArConf = confidence;
    ++mArN;
}

void MinuteEpoch::dailyHeart(float ahr, float rhr)
{
    const int64_t a = scaled(ahr, 10);
    const int64_t r = scaled(rhr, 10);
    if (a != kNoValue && a >= 0 && a < 3000) {
        mAhrX10 = static_cast<int32_t>(a);
    }
    if (r != kNoValue && r >= 0 && r < 3000) {
        mRhrX10 = static_cast<int32_t>(r);
    }
}

namespace
{
// The columns, in order. Changing these changes every file; keep probe_report.py in step.
const char kHeader[] =
    "utc,local,uptime_s,mode,period,"
    "batt_pct_x100,charging,usb,batt_mv,batt_ma_x100,batt_avg_ma_x100,batt_mah_x10,batt_n,wakes,"
    "acc_n,acc_mag_x100,enmo_mg_x10,"
    "ang0,ang1,ang2,ang3,ang4,ang5,ang6,ang7,ang8,ang9,ang10,ang11,ang_change_x10,"
    "hr_n,hr_ok,hr_min,hr_mean_x10,hr_max,trust_mean_x10,hr_opt_zero,hr_src,hr_first_s,"
    "ahr_x10,rhr_x10,steps_day,active_day,floors_day,"
    "temp_x100,temp_n,touch,touch_changes,md_still,md_motion,md_sig,ar_id,ar_conf,ar_n,wrist_n,"
    "beat_n,rr_n,rr_mean_ms,rmssd_ms_x10,rr_bad\n";
} // namespace

const char* MinuteEpoch::header()
{
    return kHeader;
}

uint8_t MinuteEpoch::columns()
{
    uint8_t n = 1;
    for (const char* p = kHeader; *p != '\0'; ++p) {
        if (*p == ',') {
            ++n;
        }
    }
    return n;
}

size_t MinuteEpoch::format(char* out, size_t cap, const RowContext& ctx) const
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    out[0] = '\0';
    Out o(out, cap);

    o.i(ctx.utc); o.comma();
    o.text(ctx.local != nullptr ? ctx.local : ""); o.comma();
    o.i(ctx.uptimeS); o.comma();
    o.text(ctx.mode != nullptr ? ctx.mode : ""); o.comma();
    o.text(ctx.night ? "night" : "day"); o.comma();

    // battery
    o.maybe(mPctX100 >= 0, mPctX100); o.comma();
    o.maybe(mCharging >= 0, mCharging); o.comma();
    o.maybe(mUsb >= 0, mUsb); o.comma();
    o.maybe(mHasMetrics, mMv); o.comma();
    o.maybe(mHasMetrics, mMaX100); o.comma();
    o.maybe(mHasMetrics, mAvgMaX100); o.comma();
    o.maybe(mHasMetrics, mMahX10); o.comma();
    o.i(mBattN); o.comma();
    o.i(mWakes); o.comma();

    // motion
    o.i(mAccN); o.comma();
    o.maybe(mAccN > 0, mAccN > 0 ? roundToInt(mMagSum * 100.0 / mAccN) : 0); o.comma();
    o.maybe(mAccN > 0 && mAccelMs2, mAccN > 0 ? roundToInt(mEnmoSum * 10.0 / mAccN) : 0); o.comma();
    int16_t prev = mLastAngle;
    int64_t change = 0;
    bool    anyAngle = false;
    for (uint8_t b = 0; b < kAngles; ++b) {
        int16_t a = kNoAngle;
        if (mBn[b] > 0) {
            a = armAngleX10(mBx[b] / mBn[b], mBy[b] / mBn[b], mBz[b] / mBn[b]);
        }
        if (a != kNoAngle) {
            if (prev != kNoAngle) {
                change += a > prev ? a - prev : prev - a;
            }
            prev = a;
            anyAngle = true;
        }
        o.maybe(a != kNoAngle, a); o.comma();
    }
    o.maybe(anyAngle, change); o.comma();

    // heart
    o.i(mHrN); o.comma();
    o.i(mHrOk); o.comma();
    o.maybe(mHrOk > 0, mHrMin); o.comma();
    o.maybe(mHrOk > 0, mHrOk > 0 ? roundToInt(mHrSum * 10.0 / mHrOk) : 0); o.comma();
    o.maybe(mHrOk > 0, mHrMax); o.comma();
    o.maybe(mHrN > 0, mHrN > 0 ? roundToInt(mTrustSum * 10.0 / mHrN) : 0); o.comma();
    o.i(mOptZero); o.comma();
    o.maybe(mHrSource != 0xFF, mHrSource); o.comma();
    o.maybe(mHrFirstMs >= 0, (mHrFirstMs + 500) / 1000); o.comma();

    // the kernel's daily figures
    o.maybe(mAhrX10 >= 0, mAhrX10); o.comma();
    o.maybe(mRhrX10 >= 0, mRhrX10); o.comma();
    o.maybe(mSteps >= 0, mSteps); o.comma();
    o.maybe(mActive >= 0, mActive); o.comma();
    o.maybe(mFloors != INT32_MIN, mFloors); o.comma();

    // body and events
    o.maybe(mTempN > 0, mTempN > 0 ? roundToInt(mTempSum * 100.0 / mTempN) : 0); o.comma();
    o.i(mTempN); o.comma();
    o.maybe(mTouch >= 0, mTouch); o.comma();
    o.i(mTouchChanges); o.comma();
    o.i(mMd[0]); o.comma();
    o.i(mMd[1]); o.comma();
    o.i(mMd[2]); o.comma();
    o.maybe(mArId != 0xFFFFFFFFu, mArId); o.comma();
    o.maybe(mArId != 0xFFFFFFFFu, mArConf); o.comma();
    o.i(mArN); o.comma();
    o.i(mWristN); o.comma();

    // beats
    o.i(mBeatN); o.comma();
    o.i(mRrN); o.comma();
    o.maybe(mRrN > 0, mRrN > 0 ? roundToInt(mRrSum / mRrN) : 0); o.comma();
    o.maybe(mRrDiffN > 0, mRrDiffN > 0 ? roundToInt(std::sqrt(mRrDiffSq / mRrDiffN) * 10.0) : 0); o.comma();
    o.i(mRrBad);
    o.text("\n");

    if (!o.ok()) {
        out[0] = '\0';
        return 0;
    }
    return o.len();
}

} // namespace EnergyProbe
