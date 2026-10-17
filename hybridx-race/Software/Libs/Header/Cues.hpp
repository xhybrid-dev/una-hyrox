/**
 ******************************************************************************
 * @file    Cues.hpp
 * @date    15-10-2026
 * @author  HybridX
 * @brief   The sound of each race event: beeps and pitch. Pure C++.
 ******************************************************************************
 *
 * Brief 8.3 gives each event its own shape: a run is one beep, a station two,
 * a Roxzone one short, the finish one long. SDK v1.6 lets a buzzer note carry a
 * pitch, so each event now also has its own pitch.
 *
 * The beep counts and lengths are unchanged and still do the telling-apart:
 * firmware that predates pitch plays every note at its default pitch (SDK
 * v1.6 CommandMessages.hpp, RequestBuzzerPlay::Note), and then the cues differ
 * by rhythm alone, as they did before.
 *
 * Pitches are notes of the C scale, inside the 1047-8372 Hz range UNA's own
 * zone alerts use (SDK commit 490da762): run C7, station G7, Roxzone C8, finish
 * C6. They are a starting point to be tuned by ear on the watch: nothing here
 * has been heard yet.
 *
 ******************************************************************************
 */
#ifndef RACE_CUES_HPP
#define RACE_CUES_HPP

#include <cstdint>

#include "RaceData.hpp"

namespace Race
{
namespace Cues
{

/// The pitch range the kernel honours; anything else plays its default.
constexpr uint16_t kPitchMinHz = 500u;
constexpr uint16_t kPitchMaxHz = 20000u;

/// One sound: @c count beeps of @c beepMs, @c silenceMs apart, at @c pitchHz.
struct Cue
{
    uint16_t beepMs;
    uint8_t  count;      ///< 0 = no sound
    uint16_t silenceMs;
    uint16_t pitchHz;    ///< 0 = the kernel's default pitch
};

constexpr Cue kRun{150u, 1u, 100u, 2093u};      ///< C7, one beep
constexpr Cue kStation{150u, 2u, 100u, 3136u};  ///< G7, two beeps
constexpr Cue kRoxzone{80u, 1u, 100u, 4186u};   ///< C8, one short beep
constexpr Cue kFinish{400u, 1u, 100u, 1047u};   ///< C6, one long beep
constexpr Cue kNone{0u, 0u, 0u, 0u};

/**
 * @brief The cue for entering a segment.
 *
 * @param type The segment being entered.
 * @retval Its cue; @c kNone for a value outside the enum, so a corrupt value
 *         makes no sound rather than the wrong one.
 */
constexpr Cue forSegment(SegmentType type)
{
    switch (type) {
    case SegmentType::Run:     return kRun;
    case SegmentType::Station: return kStation;
    case SegmentType::RoxIn:
    case SegmentType::RoxOut:  return kRoxzone;
    }
    return kNone;
}

}  // namespace Cues
}  // namespace Race

#endif  // RACE_CUES_HPP
