/**
 ******************************************************************************
 * @file    Vo2Rescore.hpp
 * @brief   HybridX Run: scoring the kept runs again when the athlete's numbers
 *          or the method change (docs/NOTES.md, "VO2max switched on").
 *
 * Each run in the history keeps its windows in a window file (Vo2WindowFile's
 * format) in the app's Runs/ folder: every window a run of Run's own built, or
 * the minutes a Trail run kept. When the entered numbers (birth date, max HR,
 * resting HR) or kMethodVersion change, every kept run is judged again with
 * today's profile and the history's figures follow. A run whose file is
 * missing (saved before this build) keeps its figure; one that no longer has
 * an estimate leaves the history. Pure C++: the service reads the files.
 ******************************************************************************
 */

#ifndef RUN_VO2_RESCORE_HPP
#define RUN_VO2_RESCORE_HPP

#include <cstddef>
#include <cstdint>

#include "Vo2History.hpp"
#include "Vo2Profile.hpp"
#include "Vo2Run.hpp"

namespace RunVo2
{

/// Raised when a change to the method should score every kept run again. 1: the HR-lag guard and
/// the 42-day limit switched on (28 October 2026).
inline constexpr uint8_t kMethodVersion = 1;

/// What the runs are scored with, as one number: the method and what the athlete entered. Not the
/// watch's resting HR (it changes daily) nor the auto max (it follows the runs). Never 0.
uint32_t profileKey(const ProfileInput& in);

/// The window file's name for a kept run: "Runs/<utc>.json".
size_t runFilePath(char* out, size_t cap, uint32_t utc);

enum class RescoreOutcome : uint8_t {
    Scored,     ///< a figure: vo2x10 and windows are set
    NoFigure,   ///< the windows no longer give one: drop the run
    NoFile,     ///< nothing readable: keep the run as it is
};

/// Judge one kept run's window file again. @p keptOnly for a Trail run's (its file holds only
/// the minutes Trail kept). @p run is scratch: it is reset.
RescoreOutcome rescoreRun(const char* text, size_t len, bool keptOnly, const Profile& profile,
                          Vo2Run& run, uint16_t& vo2x10, uint16_t& windows);

}  // namespace RunVo2

#endif  // RUN_VO2_RESCORE_HPP
