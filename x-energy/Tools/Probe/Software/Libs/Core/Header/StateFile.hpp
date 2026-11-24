/**
 ******************************************************************************
 * @file    StateFile.hpp
 * @brief   The Energy Probe's state.txt: the settings, the rotation's first
 *          night, the override, the night in progress and the last nights'
 *          results. One `key=value` per line, integers only.
 *
 * Pure. Reading is forgiving: unknown keys are skipped, a bad value keeps its
 * default, and text without the version line reads as nothing saved.
 ******************************************************************************
 */

#ifndef ENERGY_PROBE_STATE_FILE_HPP
#define ENERGY_PROBE_STATE_FILE_HPP

#include <cstddef>
#include <cstdint>

#include "NightSummary.hpp"
#include "Schedule.hpp"

namespace EnergyProbe
{

constexpr uint8_t kHistory = 7;

struct ProbeState {
    Settings   settings;
    NightOpen  night;
    NightBrief history[kHistory];   ///< newest first
    uint8_t    historyCount = 0;
    uint32_t   starts       = 0;    ///< how many times the service has started
};

/// Add a closed night to the front of the history.
void pushHistory(ProbeState& s, const NightBrief& b);

/// The file's text. Returns its length; 0 if `cap` is too small.
size_t writeState(char* out, size_t cap, const ProbeState& s);

/// Parse `len` bytes. False (and `out` left at its defaults) without the version line.
bool readState(const char* text, size_t len, ProbeState& out);

} // namespace EnergyProbe

#endif // ENERGY_PROBE_STATE_FILE_HPP
