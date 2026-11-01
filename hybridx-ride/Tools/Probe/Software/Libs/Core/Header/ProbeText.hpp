/**
 ******************************************************************************
 * @file    ProbeText.hpp
 * @brief   The bike probe's steps and lines (hybridx-ride/docs/PROBE.md).
 *
 * The probe asks the kernel's accessory contract for each kind in turn and
 * records what comes back. SDK v1.6.2 defines one kind, HRM (bit 0); bits 1
 * and 2 are commented out in SDK/Messages/AccessoryMessages.hpp as "Reserved
 * for future profiles (do not reuse): CADENCE = 1u << 1, POWER = 1u << 2". The
 * probe sends them as plain numbers to see whether this firmware answers; it
 * defines no API of its own.
 *
 * Header-only: the GUI shows the lines the service makes. Integers only.
 ******************************************************************************
 */

#ifndef RIDE_PROBE_TEXT_HPP
#define RIDE_PROBE_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace RideProbe
{

/// The requests, in order: the heart-rate control first, then the reserved bits.
struct Step {
    uint32_t    kinds;
    const char* label;   ///< for the screen, at most 18 characters
};

inline constexpr Step kSteps[] = {
    {0x1u, "HRM (control)"},
    {0x2u, "bit 1 (cadence?)"},
    {0x4u, "bit 2 (power?)"},
    {0x6u, "bits 1+2"},
    {0x7u, "bits 0+1+2"},
};
inline constexpr uint8_t kStepCount = sizeof(kSteps) / sizeof(kSteps[0]);

/// SDK::Accessory::State, by its number (0..5); anything else reads "state N".
inline const char* stateName(uint8_t state)
{
    static const char* const kNames[] = {"UNAVAILABLE", "IDLE", "SEARCHING", "CONNECTING", "CONNECTED", "LOST"};
    return state < sizeof(kNames) / sizeof(kNames[0]) ? kNames[state] : nullptr;
}

/// Fit @p n into a line: snprintf's result held to what was written.
inline size_t clampLen(int n, size_t cap)
{
    return n < 0 ? 0 : (static_cast<size_t>(n) < cap ? static_cast<size_t>(n) : cap - 1);
}

/// The screen's line for a status event: "12.3s 0x2 SEARCHING Polar".
inline size_t statusLine(char* out, size_t cap, uint32_t msSinceRequest, uint32_t kind, uint8_t state,
                         const char* name)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    const char* s = stateName(state);
    char sbuf[12];
    if (s == nullptr) {
        std::snprintf(sbuf, sizeof(sbuf), "state %u", static_cast<unsigned>(state));
        s = sbuf;
    }
    const int n = std::snprintf(out, cap, "%lu.%lus 0x%lx %s%s%s",
                                static_cast<unsigned long>(msSinceRequest / 1000u),
                                static_cast<unsigned long>((msSinceRequest % 1000u) / 100u),
                                static_cast<unsigned long>(kind), s, (name && name[0]) ? " " : "",
                                name ? name : "");
    return clampLen(n, cap);
}

/// The CSV's header and a row: ms since the probe started, the event, the request's kinds,
/// the event's kind, state and name, and the heart rate's source and external bpm.
inline constexpr const char* kCsvHeader = "ms,event,request,kind,state,name,hr_source,ext_bpm\n";

inline size_t csvRow(char* out, size_t cap, uint32_t ms, const char* event, uint32_t request, uint32_t kind,
                     int state, const char* name, int hrSource, int extBpm)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    // The device name goes in quotes, with any quote or comma in it replaced, so the row stays a row.
    char clean[25] = {};
    for (size_t i = 0; name && name[i] && i < sizeof(clean) - 1; ++i) {
        const char c = name[i];
        clean[i] = (c == '"' || c == ',' || c == '\n' || c == '\r') ? '_' : c;
    }
    const int n = std::snprintf(out, cap, "%lu,%s,0x%lx,0x%lx,%d,\"%s\",%d,%d\n", static_cast<unsigned long>(ms),
                                event ? event : "", static_cast<unsigned long>(request),
                                static_cast<unsigned long>(kind), state, clean, hrSource, extBpm);
    return clampLen(n, cap);
}

}  // namespace RideProbe

#endif  // RIDE_PROBE_TEXT_HPP
