/**
 ******************************************************************************
 * @file    StreakTypes.hpp
 * @brief   The kinds of session the streak knows, shared by every part.
 *
 * Header-only and SDK-free: the service, the glance, the GUI and the host
 * tests all use it. The names shown to the athlete live in the GUI's Coach.
 ******************************************************************************
 */

#ifndef STREAK_TYPES_HPP
#define STREAK_TYPES_HPP

#include <cstdint>

namespace Streak
{

/// What a session was (PLAN 5.3). Hybrid is a HYROX-format session: the UI
/// never uses the trademark (DESIGN 7).
enum class Kind : uint8_t {
    Run,
    Ride,
    Walk,        ///< walking and hiking
    Strength,
    Workout,
    Hybrid,
    Row,         ///< manual logs only: no SDK app records rowing
    Other,
};

constexpr uint8_t kKindCount = 8;

/// A goal's scope: every kind, or just one (PLAN 6.3, S2).
constexpr uint8_t kScopeAny = 0xFF;

} // namespace Streak

#endif // STREAK_TYPES_HPP
