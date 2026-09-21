/**
 ******************************************************************************
 * @file    RaceModel.cpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   Race state machine and timekeeping.
 ******************************************************************************
 */

#include "RaceModel.hpp"

#include <cstdio>

namespace Race
{

namespace
{

/// UTF-8 middle dot, the separator in every label (brief 7.2).
constexpr const char *kSep = "\xC2\xB7";

}  // namespace

// -- Template -----------------------------------------------------------------

uint8_t RaceModel::plannedCount(Format format, bool roxzone)
{
    const uint8_t rounds =
            static_cast<uint8_t>(lastRound(format) - firstRound(format) + 1u);

    // Roxzone on is four segments a round, less the ROX_OUT that never follows
    // the final station.
    return roxzone ? static_cast<uint8_t>(rounds * 4u - 1u)
                   : static_cast<uint8_t>(rounds * 2u);
}

uint8_t RaceModel::buildTemplate(Format format, bool roxzone, SegmentDesc *out,
                                 uint8_t capacity)
{
    const uint8_t needed = plannedCount(format, roxzone);
    if (out == nullptr || capacity < needed) {
        return 0u;
    }

    const uint8_t first = firstRound(format);
    const uint8_t last = lastRound(format);

    uint8_t n = 0u;
    for (uint8_t round = first; round <= last; ++round) {
        out[n++] = { SegmentType::Run, round, 0u };
        if (roxzone) {
            out[n++] = { SegmentType::RoxIn, round, 0u };
        }
        out[n++] = { SegmentType::Station, round, round };
        if (roxzone && round != last) {
            out[n++] = { SegmentType::RoxOut, round, 0u };
        }
    }

    return n;
}

void RaceModel::label(const SegmentDesc &desc, char *buf, size_t size)
{
    if (buf == nullptr || size == 0u) {
        return;
    }

    switch (desc.type) {
    case SegmentType::Run:
        snprintf(buf, size, "RUN %u/%u %s %s", static_cast<unsigned>(desc.round),
                 static_cast<unsigned>(kRunCount), kSep, kRunWork);
        break;

    case SegmentType::RoxIn:
        snprintf(buf, size, "%s", "ROXZONE IN");
        break;

    case SegmentType::Station:
        // stationId is 1-based; guard it because there is no MMU.
        if (desc.stationId >= 1u && desc.stationId <= kStationCount) {
            const Station &s = kStations[desc.stationId - 1u];
            snprintf(buf, size, "%s %s %s", s.name, kSep, s.work);
        } else {
            snprintf(buf, size, "%s", "STATION");
        }
        break;

    case SegmentType::RoxOut:
        snprintf(buf, size, "%s", "ROXZONE OUT");
        break;

    default:
        buf[0] = '\0';
        break;
    }
}

// -- Internals ----------------------------------------------------------------

uint32_t RaceModel::pausedSoFar(uint32_t nowMs) const
{
    uint32_t paused = mSegmentPausedMs;
    if (mState == State::Paused) {
        paused += nowMs - mPauseStartMs;
    }
    return paused;
}

void RaceModel::openSegment(uint32_t atMs)
{
    // The open segment is always mPlan[mRecordedCount]; only the accumulators
    // need clearing.
    mSegmentOpenMs = atMs;
    mSegmentPausedMs = 0u;
    mHrSum = 0u;
    mHrCount = 0u;
    mHrMax = 0u;
}

void RaceModel::closeCurrent(uint32_t atMs)
{
    if (mRecordedCount >= mPlannedCount || mRecordedCount >= kMaxSegments) {
        return;
    }

    // Absorb an in-progress pause so finishing from Paused accounts correctly.
    const uint32_t paused = pausedSoFar(atMs);
    const uint32_t wall = atMs - mSegmentOpenMs;

    SegmentResult &r = mResults[mRecordedCount];
    r.desc = mPlan[mRecordedCount];
    r.startMs = mSegmentOpenMs;
    r.pausedMs = paused;
    r.activeMs = (wall > paused) ? (wall - paused) : 0u;
    r.hrSum = mHrSum;
    r.hrCount = mHrCount;
    r.hrMax = mHrMax;

    ++mRecordedCount;

    // Clear the accumulators here rather than in openSegment(), because the
    // finishing split closes a segment without opening another. Leaving them
    // set would make a later undoFinish() merge this segment's pause and heart
    // rate in a second time.
    mSegmentPausedMs = 0u;
    mHrSum = 0u;
    mHrCount = 0u;
    mHrMax = 0u;
}

void RaceModel::reopenLast()
{
    if (mRecordedCount == 0u) {
        return;
    }

    --mRecordedCount;
    const SegmentResult &r = mResults[mRecordedCount];

    // The reopened segment keeps its original start, and swallows everything
    // accrued since it was closed. Sum and count merge losslessly.
    mSegmentOpenMs = r.startMs;
    mSegmentPausedMs += r.pausedMs;
    mHrSum += r.hrSum;
    mHrCount = static_cast<uint16_t>(mHrCount + r.hrCount);
    if (r.hrMax > mHrMax) {
        mHrMax = r.hrMax;
    }

    mResults[mRecordedCount] = SegmentResult {};
}

// -- Events -------------------------------------------------------------------

bool RaceModel::start(const Config &config, uint32_t nowMs)
{
    if (mState != State::Idle) {
        return false;
    }

    const uint8_t n = buildTemplate(config.format, config.roxzone, mPlan, kMaxSegments);
    if (n == 0u) {
        return false;
    }

    mConfig = config;
    mPlannedCount = n;
    mRecordedCount = 0u;
    mCompleted = false;
    mRaceStartMs = nowMs;
    mRaceEndMs = nowMs;

    // The lockout runs from the start too, so a double press on "Start race"
    // cannot immediately split out of segment 0.
    mLastSplitMs = nowMs;

    openSegment(nowMs);
    mState = State::Running;
    return true;
}

bool RaceModel::split(uint32_t pressMs)
{
    if (mState != State::Running) {
        return false;
    }

    // Unsigned, so this is correct across the clock wrap. Exactly at the
    // window is accepted (brief 12.1).
    if ((pressMs - mLastSplitMs) < mConfig.lockoutMs) {
        return false;
    }

    closeCurrent(pressMs);
    mLastSplitMs = pressMs;

    if (mRecordedCount >= mPlannedCount) {
        mRaceEndMs = pressMs;
        mCompleted = true;
        mState = State::Finished;
    } else {
        openSegment(pressMs);
    }

    return true;
}

bool RaceModel::undoSplit()
{
    if (mState != State::Running || mRecordedCount == 0u) {
        return false;
    }

    reopenLast();

    // The reopened segment started at the split that opened it, which is now
    // the most recent one. Restoring it this way means an immediate re-split is
    // not blocked by the press we just took back.
    mLastSplitMs = mSegmentOpenMs;
    return true;
}

bool RaceModel::pause(uint32_t nowMs)
{
    if (mState != State::Running) {
        return false;
    }
    mPauseStartMs = nowMs;
    mState = State::Paused;
    return true;
}

bool RaceModel::resume(uint32_t nowMs)
{
    if (mState != State::Paused) {
        return false;
    }
    mSegmentPausedMs += nowMs - mPauseStartMs;
    mState = State::Running;
    return true;
}

bool RaceModel::finishEarly(uint32_t nowMs)
{
    if (mState != State::Running && mState != State::Paused) {
        return false;
    }

    closeCurrent(nowMs);
    mRaceEndMs = nowMs;
    mCompleted = false;
    mState = State::Finished;
    return true;
}

bool RaceModel::undoFinish()
{
    // Only a finishing split can be taken back; a race ended early was not
    // finished by a press, so there is nothing to undo (brief 7.3).
    if (mState != State::Finished || !mCompleted || mRecordedCount == 0u) {
        return false;
    }

    mState = State::Running;
    reopenLast();
    mCompleted = false;
    mLastSplitMs = mSegmentOpenMs;
    return true;
}

bool RaceModel::save()
{
    if (mState != State::Finished) {
        return false;
    }
    mState = State::Saved;
    return true;
}

bool RaceModel::discard()
{
    if (mState != State::Running && mState != State::Paused &&
        mState != State::Finished) {
        return false;
    }
    mState = State::Discarded;
    return true;
}

void RaceModel::addHeartRate(uint8_t bpm)
{
    if (mState != State::Running) {
        return;
    }

    mHrSum += bpm;
    ++mHrCount;
    if (bpm > mHrMax) {
        mHrMax = bpm;
    }
}

// -- Queries ------------------------------------------------------------------

const SegmentDesc *RaceModel::currentSegment() const
{
    if (mState != State::Running && mState != State::Paused) {
        return nullptr;
    }
    if (mRecordedCount >= mPlannedCount) {
        return nullptr;
    }
    return &mPlan[mRecordedCount];
}

const SegmentDesc *RaceModel::nextSegment() const
{
    if (mState != State::Running && mState != State::Paused) {
        return nullptr;
    }
    const uint8_t next = static_cast<uint8_t>(mRecordedCount + 1u);
    if (next >= mPlannedCount) {
        return nullptr;
    }
    return &mPlan[next];
}

const SegmentResult *RaceModel::recorded(uint8_t index) const
{
    if (index >= mRecordedCount) {
        return nullptr;
    }
    return &mResults[index];
}

uint32_t RaceModel::currentSegmentActiveMs(uint32_t nowMs) const
{
    if (mState != State::Running && mState != State::Paused) {
        return 0u;
    }

    const uint32_t wall = nowMs - mSegmentOpenMs;
    const uint32_t paused = pausedSoFar(nowMs);
    return (wall > paused) ? (wall - paused) : 0u;
}

uint32_t RaceModel::totalActiveMs(uint32_t nowMs) const
{
    uint32_t total = 0u;
    for (uint8_t i = 0u; i < mRecordedCount; ++i) {
        total += mResults[i].activeMs;
    }
    return total + currentSegmentActiveMs(nowMs);
}

uint32_t RaceModel::totalElapsedMs(uint32_t nowMs) const
{
    if (mState == State::Idle) {
        return 0u;
    }
    if (isOver()) {
        return mRaceEndMs - mRaceStartMs;
    }
    return nowMs - mRaceStartMs;
}

uint32_t RaceModel::totalActiveMsOfType(SegmentType type) const
{
    uint32_t total = 0u;
    for (uint8_t i = 0u; i < mRecordedCount; ++i) {
        if (mResults[i].desc.type == type) {
            total += mResults[i].activeMs;
        }
    }
    return total;
}

}  // namespace Race
