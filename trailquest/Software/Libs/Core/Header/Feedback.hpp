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
 * A buzzer note here is a duration, a volume and a pitch in Hz (SDK v1.6.0,
 * RequestBuzzerPlay::Note::frequency, 500-20000). Firmware 1.6 or newer is
 * required of every watch (Jon, 21 October 2026), so the patterns are short
 * original jingles: rising for good news, falling for bad. A watch that did not play
 * the pitch would still hear the rhythm; the vibro carries the meaning either way.
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
    BossWarning,   ///< A boss tells: two alarm tones.
    StageClear,    ///< A boss is beaten.
    Finale,        ///< The summit.
    Fuel,          ///< An item used in a duel.
    Found,         ///< Something found on the trail or in a cache.
    Hut,           ///< Arriving at a hut: a rest.
    Join,          ///< A spirit joins the pack.
    Count
};

/// One vibro note: an effect (SDK RequestVibroPlay::Effect), or a pause.
struct VibroNote {
    uint8_t  effect;   ///< 0 for a pause.
    uint16_t pauseMs;  ///< Only when effect is 0.
};

/// One buzzer note: a duration, a volume (0 is silence) and a pitch.
struct BuzzerNote {
    uint16_t ms;
    uint8_t  volume;   ///< 0, 33, 66 or 100.
    uint16_t hz;       ///< 500-20000, or 0 for a rest (volume 0).
};

constexpr uint16_t kMinBuzzerHz = 500;
constexpr uint16_t kMaxBuzzerHz = 20000;

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

constexpr uint8_t kStrongBuzz            = 14;
constexpr uint8_t kMediumClick           = 21;
constexpr uint8_t kSharpTick             = 24;
constexpr uint8_t kShortDoubleMedium     = 31;
constexpr uint8_t kPulsingStrong         = 52;
constexpr uint8_t kPulsingMedium         = 54;

// Pitches, Hz (equal temperament, rounded); the buzzer plays nothing under 500.
constexpr uint16_t C5 = 523, D5 = 587, E5 = 659, F5 = 698, G5 = 784, A5 = 880, B5 = 988;
constexpr uint16_t C6 = 1047, E6 = 1319, G6 = 1568, C7 = 2093;
constexpr uint16_t Eb5 = 622, B4 = 500;   // 500 is the lowest the buzzer plays

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
        // Hit: one sharp click; two quick rising notes.
        { "Hit", 1, { {kSharpClick, 0} },
          2, { {40, 66, G5}, {70, 66, B5} } },
        // Miss: a soft bump; two falling, quiet notes.
        { "Miss", 1, { {kSoftBump, 0} },
          2, { {120, 33, Eb5}, {180, 33, B4} } },
        // Perfect: double click then a strong one; a bright run up to the top.
        { "Perfect", 3, { {kDoubleClick, 0}, {0, 80}, {kStrongClick, 0} },
          7, { {45, 66, C6}, {15, 0, 0}, {45, 66, E6}, {15, 0, 0}, {45, 100, G6}, {15, 0, 0}, {140, 100, C7} } },
        // Level up: two strong double clicks; C-E-G-C up, then a held E.
        { "Level up", 3, { {kShortDoubleStrong, 0}, {0, 120}, {kShortDoubleStrong, 0} },
          9, { {70, 66, C5}, {25, 0, 0}, {70, 66, E5}, {25, 0, 0}, {70, 66, G5}, {25, 0, 0}, {70, 100, C6}, {25, 0, 0}, {280, 100, E6} } },
        // Win: a long double click; three short G's and a long C.
        { "Win", 1, { {kLongDoubleSharpStrong, 0} },
          7, { {90, 100, G5}, {50, 0, 0}, {90, 100, G5}, {50, 0, 0}, {90, 100, G5}, {50, 0, 0}, {340, 100, C6} } },
        // Lose: the long alert; three falling notes and a long low one.
        { "Lose", 1, { {kAlert750, 0} },
          7, { {160, 66, E5}, {40, 0, 0}, {160, 66, D5}, {40, 0, 0}, {160, 66, C5}, {40, 0, 0}, {340, 33, B4} } },
        // Boss warning: a strong buzz; a two-tone alarm, twice.
        { "Boss warning", 1, { {kStrongBuzz, 0} },
          7, { {120, 100, A5}, {40, 0, 0}, {120, 100, F5}, {40, 0, 0}, {120, 100, A5}, {40, 0, 0}, {120, 100, F5} } },
        // Stage clear: a long double click; a rising fanfare ending high.
        { "Stage clear", 1, { {kLongDoubleSharpStrong, 0} },
          7, { {100, 100, G5}, {30, 0, 0}, {100, 100, C6}, {30, 0, 0}, {100, 100, E6}, {30, 0, 0}, {380, 100, G6} } },
        // Finale: the long double click, then pulsing; the fanfare, and a held top note.
        { "Finale", 3, { {kLongDoubleSharpStrong, 0}, {0, 150}, {kPulsingStrong, 0} },
          9, { {80, 66, C5}, {80, 66, G5}, {80, 100, C6}, {80, 100, E6}, {80, 100, C6}, {80, 100, E6}, {30, 0, 0}, {120, 100, G6}, {500, 100, C7} } },
        // Fuel: a medium click; two blips up.
        { "Fuel", 1, { {kMediumClick, 0} },
          3, { {50, 66, F5}, {30, 0, 0}, {70, 66, A5} } },
        // Found: a sharp tick; a little sparkle.
        { "Found", 1, { {kSharpTick, 0} },
          5, { {50, 66, C6}, {20, 0, 0}, {50, 66, E6}, {20, 0, 0}, {90, 66, G6} } },
        // Hut: slow pulses; three gentle notes, a calm rest.
        { "Hut", 1, { {kPulsingMedium, 0} },
          5, { {140, 33, C5}, {30, 0, 0}, {140, 33, E5}, {30, 0, 0}, {220, 33, G5} } },
        // Join: a medium double click; a welcome, two notes.
        { "Join", 1, { {kShortDoubleMedium, 0} },
          3, { {80, 66, G5}, {30, 0, 0}, {160, 66, C6} } },
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

/// True if @p hz is a pitch the buzzer plays (or 0, a rest).
inline bool isBuzzerHz(uint16_t hz)
{
    return hz == 0 || (hz >= kMinBuzzerHz && hz <= kMaxBuzzerHz);
}

/// True if @p volume is one the buzzer supports.
inline bool isBuzzerVolume(uint8_t volume)
{
    return volume == 0 || volume == 33 || volume == 66 || volume == 100;
}

} // namespace TrailQuest

#endif // TRAILQUEST_FEEDBACK_HPP
