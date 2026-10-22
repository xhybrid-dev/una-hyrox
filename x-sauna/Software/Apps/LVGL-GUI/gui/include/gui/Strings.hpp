/**
 ******************************************************************************
 * @file    Strings.hpp
 * @brief   The words on X Sauna's screens, in one place. British English.
 ******************************************************************************
 */

#ifndef STRINGS_HPP
#define STRINGS_HPP

#include <cstdint>

#include "SessionModel.hpp"

namespace Strings
{

constexpr const char* kAppTitle   = "X Sauna";
constexpr const char* kNoValue    = "--";

// The main menu
constexpr const char* kStart      = "Start";
constexpr const char* kMode       = "Mode";
constexpr const char* kRest       = "Rest";
constexpr const char* kModeSaunaPlunge = "Sauna + plunge";
constexpr const char* kModeJustSauna   = "Just sauna";
constexpr const char* kOn         = "On";
constexpr const char* kOff        = "Off";
constexpr const char* kAlwaysOn   = "Always on";

// What Start will do, under it
constexpr const char* kStartSaunaPlunge     = "Sauna, plunge";
constexpr const char* kStartSaunaPlungeRest = "Sauna, plunge, rest";
constexpr const char* kStartJustSauna       = "Sauna, rest";

// The action menu
constexpr const char* kActionTitle = "Session";
constexpr const char* kResume      = "Resume";
constexpr const char* kPause       = "Pause";
constexpr const char* kResumeTimer = "Resume\ntimer";
constexpr const char* kEnd         = "End\nsession";
constexpr const char* kDiscard     = "Discard";

// The live screen
constexpr const char* kPaused     = "Paused";
constexpr const char* kTotals     = "Totals";
constexpr const char* kTotal      = "Total";
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
constexpr const char* kRounds     = "Rounds";
constexpr const char* kHeartRate  = "Heart rate";
constexpr const char* kAverage    = "Average";
constexpr const char* kHighest    = "Highest";
constexpr const char* kRecovery   = "Best recovery";

/// A phase's name as the screens write it.
inline const char* phase(uint8_t p)
{
    switch (static_cast<Sauna::Phase>(p)) {
        case Sauna::Phase::Sauna:  return "Sauna";
        case Sauna::Phase::Plunge: return "Plunge";
        case Sauna::Phase::Rest:   return "Rest";
    }
    return kNoValue;
}

} // namespace Strings

#endif // STRINGS_HPP
