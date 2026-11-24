/**
 ******************************************************************************
 * @file    ProbeMode.hpp
 * @brief   The Energy Probe's recording modes, the nights' rotation and the
 *          night windows (x-energy/docs/PROBE.md).
 *
 * Header-only: the GUI process links no library source, and it shows these
 * names. No SDK, no heap.
 ******************************************************************************
 */

#ifndef ENERGY_PROBE_MODE_HPP
#define ENERGY_PROBE_MODE_HPP

#include <cstdint>

namespace EnergyProbe
{

/// What the service listens to. Off is the battery baseline.
enum class Mode : uint8_t {
    Off         = 0,   ///< battery only, a row every 10 minutes
    Full        = 1,   ///< every sensor a sleep and energy app could use
    Motion      = 2,   ///< the accelerometer only
    Heart       = 3,   ///< heart rate every second only
    HeartSparse = 4,   ///< heart rate for one minute in every five only
};
constexpr uint8_t kModeCount = 5;

inline Mode modeFrom(uint32_t v)
{
    return v < kModeCount ? static_cast<Mode>(v) : Mode::Full;
}

/// Sensor groups, as bits. Battery is in every mode.
enum Group : uint32_t {
    kGroupBattery     = 1u << 0,
    kGroupMotion      = 1u << 1,   ///< ACCELEROMETER (or ACCELEROMETER_RAW)
    kGroupHeart       = 1u << 2,   ///< HEART_RATE_EX every second
    kGroupHeartSparse = 1u << 3,   ///< HEART_RATE_EX one minute in five
    kGroupBeats       = 1u << 4,   ///< HEART_BEAT
    kGroupTemperature = 1u << 5,   ///< AMBIENT_TEMPERATURE
    kGroupEvents      = 1u << 6,   ///< TOUCH_DETECT, MOTION_DETECT, ACTIVITY_RECOGNITION, WRIST_MOTION
    kGroupDaily       = 1u << 7,   ///< the kernel's daily HR metrics, steps, active time, floors
};

inline uint32_t groups(Mode m)
{
    switch (m) {
        case Mode::Off:         return kGroupBattery;
        case Mode::Full:        return kGroupBattery | kGroupMotion | kGroupHeart | kGroupBeats |
                                       kGroupTemperature | kGroupEvents | kGroupDaily;
        case Mode::Motion:      return kGroupBattery | kGroupMotion;
        case Mode::Heart:       return kGroupBattery | kGroupHeart;
        case Mode::HeartSparse: return kGroupBattery | kGroupHeartSparse;
    }
    return kGroupBattery;
}

/// The name on the watch (at most 12 characters).
inline const char* modeName(Mode m)
{
    switch (m) {
        case Mode::Off:         return "Off";
        case Mode::Full:        return "Full";
        case Mode::Motion:      return "Motion";
        case Mode::Heart:       return "Heart";
        case Mode::HeartSparse: return "Heart 1 in 5";
    }
    return "?";
}

/// The word in the CSV files.
inline const char* modeCode(Mode m)
{
    switch (m) {
        case Mode::Off:         return "off";
        case Mode::Full:        return "full";
        case Mode::Motion:      return "motion";
        case Mode::Heart:       return "heart";
        case Mode::HeartSparse: return "heart5";
    }
    return "?";
}

/// One line on what a mode records, for the Tonight page.
inline const char* modeHint(Mode m)
{
    switch (m) {
        case Mode::Off:         return "battery only";
        case Mode::Full:        return "every sensor";
        case Mode::Motion:      return "movement only";
        case Mode::Heart:       return "heart rate only";
        case Mode::HeartSparse: return "HR 1 min in 5";
    }
    return "";
}

/// Minutes per row: the Off baseline wakes as little as it can.
inline uint32_t rowMinutes(Mode m)
{
    return m == Mode::Off ? 10u : 1u;
}

/// The nights' rotation from the first night after install: on and off alternate first
/// (the main comparison), then each part alone. Then it repeats.
constexpr uint8_t kRotationLength = 8;
constexpr Mode kRotation[kRotationLength] = {
    Mode::Full, Mode::Off, Mode::Full, Mode::Off,
    Mode::Motion, Mode::Heart, Mode::HeartSparse, Mode::Off,
};

inline Mode rotationMode(int32_t nightIndex)
{
    if (nightIndex < 0) {
        nightIndex = 0;
    }
    return kRotation[static_cast<uint32_t>(nightIndex) % kRotationLength];
}

/// The night windows offered (local hours). A night belongs to the date of its evening.
struct Window {
    uint8_t startHour;
    uint8_t endHour;
};
constexpr uint8_t kWindowCount   = 4;
constexpr uint8_t kDefaultWindow = 1;
constexpr Window  kWindows[kWindowCount] = {{21, 7}, {22, 8}, {23, 9}, {0, 10}};

inline Window window(uint8_t index)
{
    return kWindows[index < kWindowCount ? index : kDefaultWindow];
}

/// The Tonight override: 0 follows the rotation, 1 + mode forces a mode for one night.
constexpr uint8_t kOverrideAuto  = 0;
constexpr uint8_t kOverrideCount = 1 + kModeCount;

} // namespace EnergyProbe

#endif // ENERGY_PROBE_MODE_HPP
