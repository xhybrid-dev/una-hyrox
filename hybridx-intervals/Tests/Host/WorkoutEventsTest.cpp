#include <gtest/gtest.h>

#include "WorkoutEvents.hpp"

using namespace Intervals;

TEST(WorkoutEvents, AddsUpToCapacityInOrder)
{
    Events events;
    for (uint8_t i = 0; i < Events::kMax; ++i) {
        events.add(EventKind::StepStarted, i);
    }
    ASSERT_EQ(events.count, Events::kMax);
    for (uint8_t i = 0; i < Events::kMax; ++i) {
        EXPECT_EQ(events.items[i].a, i);
    }
    EXPECT_EQ(events.dropped, 0u);
}

TEST(WorkoutEvents, AZoneChangedIsDroppedWhenFullRatherThanEvictingAnything)
{
    Events events;
    for (uint8_t i = 0; i < Events::kMax; ++i) {
        events.add(EventKind::StepStarted, i);
    }
    events.add(EventKind::ZoneChanged, 1);
    EXPECT_EQ(events.count, Events::kMax) << "the incoming ZoneChanged is simply dropped";
    EXPECT_EQ(events.dropped, 1u);
    EXPECT_EQ(events.items[Events::kMax - 1].kind, EventKind::StepStarted);
}

TEST(WorkoutEvents, AMoreImportantEventEvictsAnExistingZoneChanged)
{
    Events events;
    events.add(EventKind::ZoneChanged, 0);
    for (uint8_t i = 1; i < Events::kMax; ++i) {
        events.add(EventKind::StepStarted, i);
    }
    ASSERT_EQ(events.count, Events::kMax);
    ASSERT_EQ(events.items[0].kind, EventKind::ZoneChanged);

    events.add(EventKind::WorkoutCompleted);

    EXPECT_EQ(events.count, Events::kMax);
    EXPECT_EQ(events.dropped, 1u);
    // The ZoneChanged at slot 0 is gone; everything shifted down one, and
    // the new event lands at the end.
    for (uint8_t i = 0; i < Events::kMax; ++i) {
        EXPECT_NE(events.items[i].kind, EventKind::ZoneChanged);
    }
    EXPECT_EQ(events.items[Events::kMax - 1].kind, EventKind::WorkoutCompleted);
}

TEST(WorkoutEvents, WithNoZoneChangedToEvictAnIncomingImportantEventIsStillDropped)
{
    Events events;
    for (uint8_t i = 0; i < Events::kMax; ++i) {
        events.add(EventKind::StepStarted, i);
    }
    events.add(EventKind::WorkoutCompleted);
    EXPECT_EQ(events.count, Events::kMax);
    EXPECT_EQ(events.dropped, 1u);
    EXPECT_EQ(events.items[Events::kMax - 1].kind, EventKind::StepStarted)
        << "no ZoneChanged slot to evict, so the incoming event is dropped too";
}
