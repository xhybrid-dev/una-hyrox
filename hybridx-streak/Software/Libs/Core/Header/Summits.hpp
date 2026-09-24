/**
 ******************************************************************************
 * @file    Summits.hpp
 * @brief   The mountain ladder and the session badges (docs/DESIGN.md).
 *
 * Progress up the ladder is measured in cumulative weeks achieved (PLAN S10),
 * never in the current streak, so missing a week never knocks anyone back
 * down a mountain. Each mountain holds the weeks between two summits: four
 * steps up Arthur's Seat, eight more up Snowdon, and so on. Past Everest the
 * climbs go on, a fresh ascent of Everest every 52 weeks.
 *
 * Header-only and SDK-free: used by the GUI, the service and the glance.
 ******************************************************************************
 */

#ifndef STREAK_SUMMITS_HPP
#define STREAK_SUMMITS_HPP

#include <cstdint>

namespace Streak
{

/// The silhouette the scene draws for a climb (SummitScene).
enum class Shape : uint8_t {
    Crag,       ///< Arthur's Seat: a low hill with a crag, no snow
    Pyramid,    ///< Snowdon: a sharp pyramid with ridges
    Massif,     ///< Ben Nevis: broad and bulky, a steep face
    Dome,       ///< Mont Blanc: a wide snow dome
    Spire,      ///< Everest: tall and steep, with a shoulder
};

struct Climb {
    const char* name;
    uint16_t    summitAt;   ///< cumulative weeks achieved at the top
    uint8_t     steps;      ///< weeks between the previous summit and this one
    Shape       shape;
};

inline constexpr Climb kClimbs[] = {
    { "Arthur's Seat",   4,  4, Shape::Crag    },
    { "Snowdon",        12,  8, Shape::Pyramid },
    { "Ben Nevis",      26, 14, Shape::Massif  },
    { "Mont Blanc",     52, 26, Shape::Dome    },
    { "Everest",       104, 52, Shape::Spire   },
};
inline constexpr uint8_t kClimbCount = sizeof(kClimbs) / sizeof(kClimbs[0]);

/// Weeks for each extra ascent of Everest after the first.
inline constexpr uint16_t kRepeatSteps = 52;

/// Where a number of cumulative weeks puts the climber.
struct ClimbPosition {
    uint8_t  climb;          ///< index into kClimbs
    uint8_t  stepsClimbed;   ///< 0 .. steps-1 on this mountain (a reached summit starts the next climb)
    uint8_t  steps;          ///< steps on this mountain
    uint16_t ascent;         ///< 1 for the first time up, 2+ for repeat Everest ascents
};

/**
 * The climb in progress after `weeksAchieved` achieved weeks. On the week a
 * summit is reached the climber stands at the foot of the next mountain; the
 * summit celebration is triggered by the transition, not by this position.
 */
constexpr ClimbPosition climbFor(uint32_t weeksAchieved)
{
    uint32_t floor = 0;
    for (uint8_t i = 0; i < kClimbCount; ++i) {
        if (weeksAchieved < kClimbs[i].summitAt) {
            return { i, static_cast<uint8_t>(weeksAchieved - floor), kClimbs[i].steps, 1 };
        }
        floor = kClimbs[i].summitAt;
    }
    const uint32_t beyond = weeksAchieved - floor;
    return { static_cast<uint8_t>(kClimbCount - 1),
             static_cast<uint8_t>(beyond % kRepeatSteps),
             static_cast<uint8_t>(kRepeatSteps),
             static_cast<uint16_t>(2 + beyond / kRepeatSteps) };
}

/// Lifetime-session badges.
struct SessionBadge {
    const char* name;
    uint16_t    sessions;
};

inline constexpr SessionBadge kSessionBadges[] = {
    { "Trailhead",     10 },
    { "Ridge Walker",  50 },
    { "Centurion",    100 },
    { "Mountaineer",  250 },
};

} // namespace Streak

#endif // STREAK_SUMMITS_HPP
