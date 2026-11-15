/**
 ******************************************************************************
 * @file    RunVo2Max.hpp
 * @brief   X Run's VO2max, as X Run leaves it for the other apps (TP4; brief 10.3).
 *
 * HybridX Run writes ../SharedData/HybridX/vo2max.json (its Vo2History.hpp,
 * toSharedJson):
 *
 *     {"v":2,"vo2maxX10":523,"runs":5,"utc":1791000000,"beforeX10":519,
 *      "trend":[505,512,519,523]}
 *
 * X Training shows the figure on its Plan screen and keeps the one it first saw
 * in a plan as the VO2max block's second measure. It is never used for paces: it
 * is on the ACSM scale, not Daniels' (hybridx-run/docs/RACE_PREDICTIONS.md 4.3).
 * The SDK's reader does no range check, so every value is checked here; a
 * missing, malformed or newer file, or one with no runs, reads as no figure.
 ******************************************************************************
 */

#ifndef XTRAINING_RUN_VO2MAX_HPP
#define XTRAINING_RUN_VO2MAX_HPP

#include <cstddef>
#include <cstdint>

namespace XTraining {

namespace RunVo2Max {

/// Where X Run leaves it. A fixed name: nothing lists the shared folder.
inline constexpr const char* kPath = "../SharedData/HybridX/vo2max.json";

/// X Run's plausible range, x 10 (its Vo2Config kMinPlausibleVo2 and kMaxPlausibleVo2).
inline constexpr uint16_t kMinX10 = 150;
inline constexpr uint16_t kMaxX10 = 950;

/// A buffer big enough for the file (Vo2History::kMaxSharedJsonBytes is 156).
inline constexpr size_t kMaxBytes = 256;

struct Figure {
    uint16_t x10  = 0;   ///< 0 = none
    uint8_t  runs = 0;   ///< the runs it is from
    uint32_t utc  = 0;   ///< the latest run's time
};

/// Read vo2max.json (versions 1 and 2). False, and @p out left at none, for anything wrong.
bool read(const char* text, size_t len, Figure& out);

}  // namespace RunVo2Max

}  // namespace XTraining

#endif  // XTRAINING_RUN_VO2MAX_HPP
