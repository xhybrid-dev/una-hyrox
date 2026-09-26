/**
 ******************************************************************************
 * @file    WorkoutEvents.hpp
 * @brief   What WorkoutEngine reports each tick, for the GUI to play.
 *          Header-only, modeled on hybridx-streak's StreakEvents.hpp.
 ******************************************************************************
 */

#ifndef INTERVALS_WORKOUT_EVENTS_HPP
#define INTERVALS_WORKOUT_EVENTS_HPP

#include <cstdint>
#include <type_traits>

namespace Intervals
{

enum class EventKind : uint8_t {
    StepStarted,          ///< a: step index
    StepCompleted,        ///< a: step index
    RepeatBlockLooped,     ///< a: iterations remaining
    RepeatBlockCompleted,  ///< a: step index of the repeat marker
    WorkoutCompleted,
    ZoneChanged,           ///< a: the new ZoneEvaluator::ZoneState (as uint8_t)
};

struct Event {
    EventKind kind = EventKind::StepStarted;
    uint8_t   a    = 0;
    uint16_t  b    = 0;
};

struct Events {
    static constexpr uint8_t kMax = 8;
    Event   items[kMax] {};
    uint8_t count   = 0;
    uint8_t dropped = 0;   ///< events dropped to make room (ZoneChanged first)

    /// Add; when full, a ZoneChanged (frequent, least consequential) gives
    /// way to anything else.
    void add(EventKind kind, uint8_t a = 0, uint16_t b = 0)
    {
        if (count < kMax) {
            items[count++] = Event { kind, a, b };
            return;
        }
        ++dropped;
        if (kind == EventKind::ZoneChanged) {
            return;
        }
        for (int i = kMax - 1; i >= 0; --i) {
            if (items[i].kind == EventKind::ZoneChanged) {
                for (int j = i; j < kMax - 1; ++j) {
                    items[j] = items[j + 1];
                }
                items[kMax - 1] = Event { kind, a, b };
                return;
            }
        }
    }
};

static_assert(std::is_trivially_copyable<Events>::value, "Events travel inside a kernel message");

} // namespace Intervals

#endif // INTERVALS_WORKOUT_EVENTS_HPP
