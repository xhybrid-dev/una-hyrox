/**
 ******************************************************************************
 * @file    Strings.hpp
 * @brief   The words on HybridX Ride's screens, in one place. British English.
 ******************************************************************************
 */

#ifndef STRINGS_HPP
#define STRINGS_HPP

#include <cstdint>

namespace Strings
{

constexpr const char* kAppTitle   = "Ride";
constexpr const char* kNoValue    = "--";

// The main menu
constexpr const char* kStart      = "Start";
constexpr const char* kRideType   = "Ride";
constexpr const char* kOutdoor    = "Outdoor";
constexpr const char* kIndoor     = "Indoor";
constexpr const char* kSensor     = "Heart rate";
constexpr const char* kAutoLap    = "Auto lap";

// Start's hint (the GPS, outdoors) and the press-again warning
constexpr const char* kGpsReady   = "GPS ready";
constexpr const char* kGpsFinding = "Finding GPS";
constexpr const char* kIndoorHint = "Indoor ride";
constexpr const char* kNoGpsAgain = "No GPS: R1 again";

// The heart-rate source, under Start (SDK::Accessory::State)
constexpr const char* kStrapConnected = "Strap connected";
constexpr const char* kStrapFinding   = "Finding strap";
constexpr const char* kStrapLost      = "Strap lost";
constexpr const char* kWristHr        = "Wrist heart rate";

// The action menu
constexpr const char* kActionTitle = "Ride";
constexpr const char* kResume      = "Resume";
constexpr const char* kPause       = "Pause";
constexpr const char* kResumeTimer = "Resume\ntimer";
constexpr const char* kEnd         = "End\nride";
constexpr const char* kDiscard     = "Discard";

// The live screen
constexpr const char* kPaused     = "Paused";
constexpr const char* kTotals     = "Totals";
constexpr const char* kLapFace    = "Lap";
constexpr const char* kTime       = "Time";
constexpr const char* kDistance   = "Dist";
constexpr const char* kClimb      = "Climb";
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

// Units: metric, then imperial (the watch profile's choice)
constexpr const char* kKmh        = "km/h";
constexpr const char* kMph        = "mph";
constexpr const char* kKm         = "km";
constexpr const char* kMiles      = "mi";
constexpr const char* kMetres     = "m";
constexpr const char* kFeet       = "ft";

// The summary
constexpr const char* kSummary    = "Summary";
constexpr const char* kRideSummary = "Ride";
constexpr const char* kLaps       = "Laps";
constexpr const char* kHeartRate  = "Heart rate";
constexpr const char* kAverage    = "Average";
constexpr const char* kHighest    = "Highest";

} // namespace Strings

#endif // STRINGS_HPP
