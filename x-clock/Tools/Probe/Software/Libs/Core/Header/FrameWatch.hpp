/**
 ******************************************************************************
 * @file    FrameWatch.hpp
 * @brief   What the face's GUI says about its own ticking.
 *
 * The GUI counts frames and sends a report every 50 of them (5 s at the
 * watch's 10 a second). The service cannot see the screen, so it infers from
 * the reports: the frame rate over the last interval, and the gaps between
 * reports. A gap with nothing in it is the GUI not being ticked: the screen
 * off, or another app in front (Docs/service-lifecycle.md section 6: "do not
 * rely on receiving ticks while suspended").
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_FRAME_WATCH_HPP
#define CLOCK_PROBE_FRAME_WATCH_HPP

#include <cstdint>

namespace ClockProbe
{

class FrameWatch
{
public:
    /// A report arrived at @p nowMs, carrying the GUI's running totals.
    void onReport(uint32_t nowMs, uint32_t frames, uint16_t resumes, uint16_t suspends)
    {
        if (mHave) {
            const uint32_t dt = static_cast<uint32_t>(nowMs - mLastMs);
            if (frames >= mLastFrames && dt > 0) {
                // frames * 10000 / ms is the rate in tenths of a frame a second (a report is 50 frames).
                const uint32_t df = frames - mLastFrames;
                mFpsX10 = df > 400000u ? 0xFFFFu : static_cast<uint16_t>(df * 10000u / dt);
                mHaveRate = true;
            } else {
                mHaveRate = false;   // the GUI restarted: its counter began again
            }
            if (dt > mMaxGapMs) {
                mMaxGapMs = dt;
            }
        }
        mLastMs     = nowMs;
        mLastFrames = frames;
        mResumes    = resumes;
        mSuspends   = suspends;
        mHave       = true;
        ++mReports;
        ++mReportsMinute;
    }

    bool     have() const { return mHave; }
    bool     haveRate() const { return mHaveRate; }
    uint16_t fpsX10() const { return mFpsX10; }
    uint16_t resumes() const { return mResumes; }
    uint16_t suspends() const { return mSuspends; }
    uint32_t reports() const { return mReports; }
    uint32_t sinceReportMs(uint32_t nowMs) const { return mHave ? static_cast<uint32_t>(nowMs - mLastMs) : 0xFFFFFFFFu; }

    /// The minute's figures, then a fresh minute: reports that arrived, and the longest gap between two.
    uint16_t reportsThisMinute() const { return mReportsMinute; }
    uint32_t maxGapMs() const { return mMaxGapMs; }
    void     startMinute() { mReportsMinute = 0; mMaxGapMs = 0; }

private:
    bool     mHave      = false;
    bool     mHaveRate  = false;
    uint32_t mLastMs    = 0;
    uint32_t mLastFrames = 0;
    uint16_t mFpsX10    = 0;
    uint16_t mResumes   = 0;
    uint16_t mSuspends  = 0;
    uint32_t mReports   = 0;
    uint16_t mReportsMinute = 0;
    uint32_t mMaxGapMs  = 0;
};

} // namespace ClockProbe

#endif // CLOCK_PROBE_FRAME_WATCH_HPP
