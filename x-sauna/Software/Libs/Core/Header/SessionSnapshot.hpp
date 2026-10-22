/**
 ******************************************************************************
 * @file    SessionSnapshot.hpp
 * @brief   The running session as the screens show it: one plain struct the
 *          service fills once a second and sends. Pure C++, header-only.
 ******************************************************************************
 */

#ifndef SAUNA_SESSION_SNAPSHOT_HPP
#define SAUNA_SESSION_SNAPSHOT_HPP

#include <cstdint>

#include "SessionModel.hpp"

namespace Sauna {

/// Battery level when the watch has not said.
constexpr uint8_t kBatteryUnknown = 255;

struct SessionSnapshot {
    uint8_t  state = 0;           ///< RunState
    uint8_t  phase = 0;           ///< Phase
    uint8_t  round = 1;
    uint8_t  laps = 0;
    uint8_t  mode = 0;            ///< Mode
    uint8_t  rest = 0;
    uint16_t hr = 0;              ///< bpm, 0 none
    /// The recovery of the latest Sauna lap, out of the heat (so only while the
    /// phase is not Sauna): its state (Recovery::State), the seconds of the
    /// minute still to wait while Pending, and the fall in bpm once Done.
    uint8_t  recoveryState = 0;
    uint8_t  recoverySecLeft = 0;
    int16_t  recoveryDrop = 0;
    uint8_t  logOk = 1;           ///< 0 when a write to the activity file failed
    uint8_t  hour = 0;            ///< Local time of day
    uint8_t  minute = 0;
    uint8_t  battery = kBatteryUnknown;   ///< 0 to 100
    uint32_t lapMs = 0;           ///< Active time in this lap
    uint32_t activeMs = 0;
    uint32_t elapsedMs = 0;
    uint32_t phaseMs[3] = {};     ///< Active time so far in sauna, plunge, rest
};

/// What a finished session came to, for the summary screen.
struct SavedSession {
    uint8_t        saved = 0;     ///< 1 when the activity file was written
    uint32_t       activeMs = 0;
    SessionSummary summary;
};

/// A lap just ended (R2): what it was, for the lap toast.
struct LapEventData {
    uint8_t  finishedPhase = 0;   ///< Phase of the lap that ended
    uint8_t  finishedRound = 1;
    uint8_t  nextPhase = 0;       ///< Phase now begun
    uint8_t  nextRound = 1;
    uint16_t hrAvg = 0;           ///< bpm, 0 none
    uint32_t durationMs = 0;
};

/// One lap of a finished session, for the summary's lap pages.
struct LapRow {
    uint8_t  phase = 0;
    uint8_t  round = 1;
    uint8_t  recoveryState = 0;   ///< Recovery::State
    int16_t  recoveryDrop = 0;
    uint16_t hrAvg = 0;
    uint32_t durationMs = 0;
};

/// Laps on one summary screen: each takes two lines, and three fit the round face.
constexpr uint8_t kLapsPerPage = 3;

struct LapsPageData {
    uint8_t first = 0;            ///< Index of rows[0]
    uint8_t total = 0;            ///< Laps in the session
    uint8_t count = 0;            ///< Rows filled, at most kLapsPerPage
    LapRow  rows[kLapsPerPage];
};

} // namespace Sauna

#endif // SAUNA_SESSION_SNAPSHOT_HPP
