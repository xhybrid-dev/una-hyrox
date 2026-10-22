/**
 ******************************************************************************
 * @file    SessionSnapshot.hpp
 * @brief   The running session as the screen shows it: one plain struct the
 *          service fills once a second and sends. Pure C++, header-only.
 ******************************************************************************
 */

#ifndef SAUNA_SESSION_SNAPSHOT_HPP
#define SAUNA_SESSION_SNAPSHOT_HPP

#include <cstdint>

#include "SessionModel.hpp"

namespace Sauna {

struct SessionSnapshot {
    uint8_t  state = 0;           ///< RunState
    uint8_t  phase = 0;           ///< Phase
    uint8_t  round = 1;
    uint8_t  laps = 0;
    uint8_t  mode = 0;            ///< Mode
    uint8_t  rest = 0;
    uint16_t hr = 0;              ///< bpm, 0 none
    uint8_t  prevRecovery = 0;    ///< Recovery::State of the lap before this one
    uint8_t  logOk = 1;           ///< 0 when a write to the activity file failed
    int16_t  prevDrop = 0;        ///< That lap's recovery, bpm
    uint32_t lapMs = 0;           ///< Active time in this lap
    uint32_t activeMs = 0;
    uint32_t elapsedMs = 0;
};

/// What a finished session came to, for the summary screen.
struct SavedSession {
    uint8_t        saved = 0;     ///< 1 when the activity file was written
    SessionSummary summary;
};

} // namespace Sauna

#endif // SAUNA_SESSION_SNAPSHOT_HPP
