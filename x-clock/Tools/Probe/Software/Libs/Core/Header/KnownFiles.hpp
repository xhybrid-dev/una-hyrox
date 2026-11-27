/**
 ******************************************************************************
 * @file    KnownFiles.hpp
 * @brief   The files the probe looks for in the shared folder, by their exact
 *          names. The family's rule is to open fixed names and never to list
 *          (hybridx-link/docs/ACTIVITY_LINK.md); the probe also lists the two
 *          folders, once, to find out whether a face can, and what else is there.
 *
 * Nothing private is read: weighins.csv (X Weigh-in's log) and stride.json
 * (the SDK's) are asked about for their size and age only, and so is streak.json
 * (Streak's own state), which the probe has no use for.
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_KNOWN_FILES_HPP
#define CLOCK_PROBE_KNOWN_FILES_HPP

#include <cstdint>

namespace ClockProbe
{

/// What a file is, so the scan knows which reader to hand it to.
enum class Role : uint8_t {
    Link,            ///< act_<slot>.json: an activity app's last sixteen activities
    StreakNow,       ///< streak_now.json: this week's goal, and the load window
    TrainingToday,   ///< training_today.json: the plan's days
    Vo2Max,          ///< vo2max.json: X Run's figure and its trend
    SizeOnly,        ///< asked about, never opened
};

struct Known {
    const char* path;     ///< as an app opens it: relative to its own folder
    const char* label;    ///< for the log, 15 characters at most
    Role        role;
};

inline constexpr const char* kSharedDir  = "../SharedData";
inline constexpr const char* kHybridxDir = "../SharedData/HybridX";

/// The first six are the activity apps' files, in HybridXLink::kSlotPaths' order (a host test checks).
inline constexpr uint8_t kLinkFiles  = 6;
inline constexpr uint8_t kKnownCount = 12;

inline constexpr Known kKnown[kKnownCount] = {
    {"../SharedData/HybridX/act_run.json",       "act_run",        Role::Link},
    {"../SharedData/HybridX/act_race.json",      "act_race",       Role::Link},
    {"../SharedData/HybridX/act_trail.json",     "act_trail",      Role::Link},
    {"../SharedData/HybridX/act_ride.json",      "act_ride",       Role::Link},
    {"../SharedData/HybridX/act_workout.json",   "act_workout",    Role::Link},
    {"../SharedData/HybridX/act_training.json",  "act_training",   Role::Link},
    {"../SharedData/HybridX/streak_now.json",    "streak_now",     Role::StreakNow},
    {"../SharedData/HybridX/training_today.json","training_today", Role::TrainingToday},
    {"../SharedData/HybridX/vo2max.json",        "vo2max",         Role::Vo2Max},
    {"../SharedData/HybridX/streak.json",        "streak",         Role::SizeOnly},
    {"../SharedData/HybridX/weighins.csv",       "weighins",       Role::SizeOnly},
    {"../SharedData/stride.json",                "stride",         Role::SizeOnly},
};

/// Where the files the screen reads from sit in kKnown (a host test checks each against its label).
inline constexpr uint8_t kIdxStreakNow = 6;
inline constexpr uint8_t kIdxToday     = 7;
inline constexpr uint8_t kIdxVo2       = 8;

/// The biggest file the probe reads: an activity file is under 1024 bytes, training_today.json
/// under 1536 (TodayFile::kMaxJsonBytes), the others under 900.
inline constexpr uint16_t kScratchBytes = 1664;

} // namespace ClockProbe

#endif // CLOCK_PROBE_KNOWN_FILES_HPP
