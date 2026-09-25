/**
 ******************************************************************************
 * @file    StreakEvents.hpp
 * @brief   The moments the model reports and the GUI plays (DESIGN 6).
 *          Header-only: the GUI receives them in a message.
 ******************************************************************************
 */

#ifndef STREAK_EVENTS_HPP
#define STREAK_EVENTS_HPP

#include <cstdint>
#include <type_traits>

namespace Streak
{

/// Things the GUI plays, in order (DESIGN 6).
enum class EventKind : uint8_t {
    SessionFound,   ///< a: Kind, b: minutes (0 = manual)
    StepUp,         ///< this week met its target; b: weeks achieved (live)
    Summit,         ///< a: the climb reached (kClimbs index), b: weeks achieved
    Badge,          ///< a: kSessionBadges index
    BestWeek,       ///< a: sessions this week
    ShieldEarned,   ///< a: shields now
    WeekResult,     ///< last week closed; a: its count, b: target | outcome << 8
    ShieldOffer,    ///< a: missed weeks, b: the streak at stake
    StreakReset,    ///< b: the streak that ended
};

struct Event {
    EventKind kind = EventKind::SessionFound;
    uint8_t   a    = 0;
    uint16_t  b    = 0;
};

struct Events {
    static constexpr uint8_t kMax = 8;
    Event   items[kMax] {};
    uint8_t count   = 0;
    uint8_t dropped = 0;   ///< events dropped to make room (session toasts first)

    /// Add; when full, a session toast gives way to anything else.
    void add(EventKind kind, uint8_t a = 0, uint16_t b = 0)
    {
        if (count < kMax) {
            items[count++] = Event { kind, a, b };
            return;
        }
        ++dropped;
        if (kind == EventKind::SessionFound) {
            return;
        }
        for (int i = kMax - 1; i >= 0; --i) {
            if (items[i].kind == EventKind::SessionFound) {
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

} // namespace Streak

#endif // STREAK_EVENTS_HPP
