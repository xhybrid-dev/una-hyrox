/**
 ******************************************************************************
 * @file    Strings.hpp
 * @brief   The words on HybridX Workout's screens, in one place. British English.
 ******************************************************************************
 */

#ifndef STRINGS_HPP
#define STRINGS_HPP

#include <cstdint>

namespace Strings
{

constexpr const char* kAppTitle   = "Workout";
constexpr const char* kNoValue    = "--";

// The main menu
constexpr const char* kStart      = "Start";
constexpr const char* kAutoLap    = "Auto lap";

// The heart-rate source, under Start (SDK::Accessory::State)
constexpr const char* kStrapConnected = "Strap connected";
constexpr const char* kStrapFinding   = "Finding strap";
constexpr const char* kStrapLost      = "Strap lost";
constexpr const char* kWristHr        = "Wrist heart rate";

// The action menu
constexpr const char* kActionTitle = "Workout";
constexpr const char* kResume      = "Resume";
constexpr const char* kPause       = "Pause";
constexpr const char* kResumeTimer = "Resume\ntimer";
constexpr const char* kEnd         = "End\nworkout";
constexpr const char* kDiscard     = "Discard";

// The live screen
constexpr const char* kPaused     = "Paused";
constexpr const char* kTotals     = "Totals";
constexpr const char* kZones      = "Zones";
constexpr const char* kTotal      = "Total";
constexpr const char* kCalories   = "kcal";
constexpr const char* kLapsRow    = "Laps";
constexpr const char* kAutoLapRow = "Auto";
constexpr const char* kClock      = "Time";
constexpr const char* kFileTrouble = "File error";

// Hold to confirm
constexpr const char* kHoldEnd     = "Hold to\nend";
constexpr const char* kHoldDiscard = "Hold to\ndiscard";

// The result
constexpr const char* kSaving     = "Saving";
constexpr const char* kSaved      = "Saved";
constexpr const char* kNotSaved   = "Not saved";
constexpr const char* kDiscarded  = "Discarded";
constexpr const char* kSavedNote  = "It will sync to\nthe UNA app";
constexpr const char* kNotSavedNote = "The file could\nnot be written";
constexpr const char* kDiscardedNote = "Nothing was\nkept";

// The summary
constexpr const char* kSummary    = "Summary";
constexpr const char* kLaps       = "Laps";
constexpr const char* kHeartRate  = "Heart rate";
constexpr const char* kAverage    = "Average";
constexpr const char* kHighest    = "Highest";

} // namespace Strings

#endif // STRINGS_HPP
