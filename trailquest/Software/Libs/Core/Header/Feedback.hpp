/**
 ******************************************************************************
 * @file    Feedback.hpp
 * @brief   The vibration melodies and buzzer rhythms for the game's moments.
 *
 * Plain data, so it is host-tested for what the kernel will refuse: at most 8
 * vibro notes and 10 buzzer notes (una-sdk SDK/Messages/CommandMessages.hpp,
 * RequestVibroPlay / RequestBuzzerPlay), only the DRV2605 effect numbers the SDK
 * names, and only the buzzer volumes it supports (0, 33, 66, 100).
 *
 * Header-only: the GUI process links none of the Libs sources
 * (hybridx-race NOTES 3.4), and it needs the pattern names.
 *
 * The buzzer has no pitch: a note is a duration and a volume. The sounds are
 * therefore rhythms; the vibro carries most of the meaning.
 ******************************************************************************
 */

#ifndef TRAILQUEST_FEEDBACK_HPP
#define TRAILQUEST_FEEDBACK_HPP

#include <cstdint>

namespace TrailQuest
{

constexpr uint8_t kMaxVibroNotes  = 8;
constexpr uint8_t kMaxBuzzerNotes = 10;

/// The moments that get feedback.
enum class FxId : uint8_t {
    Hit = 0,
    Miss,
    Perfect,
    LevelUp,
    Win,
    Lose,
    Count
};

/// One vibro note: an effect (SDK RequestVibroPlay::Effect), or a pause.
struct VibroNote {
    uint8_t  effect;   ///< 0 for a pause.
    uint16_t pauseMs;  ///< Only when effect is 0.
};

/// One buzzer note: a duration and a volume (0 is silence).
struct BuzzerNote {
    uint16_t ms;
    uint8_t  volume;   ///< 0, 33, 66 or 100.
};

struct FxPattern {
    const char*       name;
    uint8_t           vibroCount;
    VibroNote         vibro[kMaxVibroNotes];
    uint8_t           buzzerCount;
    BuzzerNote        buzzer[kMaxBuzzerNotes];
};

namespace detail
{

// Effect numbers, from SDK RequestVibroPlay::Effect.
constexpr uint8_t kStrongClick           = 1;
constexpr uint8_t kSharpClick            = 4;
constexpr uint8_t kSoftBump              = 7;
constexpr uint8_t kDoubleClick           = 10;
constexpr uint8_t kAlert750              = 15;
constexpr uint8_t kShortDoubleStrong     = 27;
constexpr uint8_t kLongDoubleSharpStrong = 37;

constexpr uint8_t kKnownEffects[] = {
    0, 1, 4, 7, 10, 14, 15, 16, 17, 21, 24, 27, 31, 34, 37, 41, 44, 47, 52, 54, 56,
};

} // namespace detail

/// The pattern for @p id; out-of-range ids give Miss's.
inline const FxPattern& fxPattern(FxId id)
{
    using namespace detail;
    // Constant-initialised, so no run-time guard.
    static const FxPattern kPatterns[static_cast<uint8_t>(FxId::Count)] = {
        // Hit: one sharp click, a short beep.
        { "Hit", 1, { {kSharpClick, 0} },
          1, { {60, 66} } },
        // Miss: a soft bump, a long quiet beep.
        { "Miss", 1, { {kSoftBump, 0} },
          1, { {160, 33} } },
        // Perfect: double click then a strong one; three beeps getting louder.
        { "Perfect", 3, { {kDoubleClick, 0}, {0, 80}, {kStrongClick, 0} },
          5, { {50, 33}, {40, 0}, {50, 66}, {40, 0}, {80, 100} } },
        // Level up: two strong double clicks; a run of short beeps and a long one.
        { "Level up", 3, { {kShortDoubleStrong, 0}, {0, 120}, {kShortDoubleStrong, 0} },
          7, { {60, 66}, {40, 0}, {60, 66}, {40, 0}, {60, 66}, {40, 0}, {220, 100} } },
        // Win: a long double click; four even beeps.
        { "Win", 1, { {kLongDoubleSharpStrong, 0} },
          7, { {80, 100}, {60, 0}, {80, 100}, {60, 0}, {80, 100}, {60, 0}, {80, 100} } },
        // Lose: the long alert; one long low-volume beep.
        { "Lose", 1, { {kAlert750, 0} },
          1, { {420, 33} } },
    };
    const uint8_t i = static_cast<uint8_t>(id);
    return kPatterns[i < static_cast<uint8_t>(FxId::Count) ? i : static_cast<uint8_t>(FxId::Miss)];
}

/// True if @p effect is one of the effect numbers the SDK defines.
inline bool isKnownVibroEffect(uint8_t effect)
{
    for (uint8_t known : detail::kKnownEffects) {
        if (known == effect) {
            return true;
        }
    }
    return false;
}

/// True if @p volume is one the buzzer supports.
inline bool isBuzzerVolume(uint8_t volume)
{
    return volume == 0 || volume == 33 || volume == 66 || volume == 100;
}

} // namespace TrailQuest

#endif // TRAILQUEST_FEEDBACK_HPP
