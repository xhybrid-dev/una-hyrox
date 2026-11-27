/**
 ******************************************************************************
 * @file    ProbeState.hpp
 * @brief   What the probe keeps between starts, in Logs/state.txt: how many
 *          times the service has started since install, and when it first did.
 *          A restart is itself a finding (the watch restarts a face's service
 *          when the user comes back to it, or after USB mode).
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_PROBE_STATE_HPP
#define CLOCK_PROBE_PROBE_STATE_HPP

#include <cstddef>
#include <cstdint>

namespace ClockProbe
{

struct ProbeState {
    uint32_t starts   = 0;
    uint32_t firstUtc = 0;   ///< 0 until the clock was set at a start
};

/// "starts=3\nfirst=1795000000\n". Returns the length.
size_t formatState(const ProbeState& s, char* out, size_t cap);

/// Read it back. Unknown lines are ignored; a file with neither key reads as the default.
bool parseState(const char* text, size_t len, ProbeState& out);

} // namespace ClockProbe

#endif // CLOCK_PROBE_PROBE_STATE_HPP
