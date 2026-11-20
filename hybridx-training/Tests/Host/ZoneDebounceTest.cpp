// X Training: the zone alerts' debounce, with the SDK's ZoneAlerts behind it.

#include <gtest/gtest.h>

#include "SDK/Workout/WorkoutAlerts.hpp"
#include "ZoneDebounce.hpp"

using SDK::Workout::Zone;
using SDK::Workout::ZoneAlerts;
using XTraining::ZoneDebounce;
using A = ZoneAlerts::Alert;

namespace {
struct Rig {
    ZoneDebounce d;
    ZoneAlerts   a;
    int below = 0, above = 0, backIn = 0;
    Rig() { a.stepStarted(); d.reset(); }
    void feed(Zone z, int seconds)
    {
        for (int i = 0; i < seconds; ++i) {
            switch (a.tick(d.tick(z))) {
            case A::Below:  ++below;  break;
            case A::Above:  ++above;  break;
            case A::BackIn: ++backIn; break;
            default: break;
            }
        }
    }
};
} // namespace

TEST(ZoneDebounce, AShortDipBelowIsSilent)
{
    Rig r;
    r.feed(Zone::In, 30);
    r.feed(Zone::Below, ZoneDebounce::kDelaySec - 1);
    r.feed(Zone::In, 30);
    EXPECT_EQ(r.below, 0);
    EXPECT_EQ(r.backIn, 0);
}

TEST(ZoneDebounce, ADipOnTheEdgeIsOneAlertNotMany)
{
    Rig r;
    r.feed(Zone::In, 30);
    // 22 s on the edge (inside the SDK 30 s repeat): 8 s out, 3 s in, twice, then 8 out.
    for (int i = 0; i < 2; ++i) {
        r.feed(Zone::Below, 8);
        r.feed(Zone::In, 3);
    }
    r.feed(Zone::Below, 8);
    EXPECT_EQ(r.below, 1);
    EXPECT_EQ(r.backIn, 0);
}

TEST(ZoneDebounce, ALongStayOutAlertsAfterTheDelayThenBackInAfterTheDelay)
{
    Rig r;
    r.feed(Zone::In, 30);
    r.feed(Zone::Above, ZoneDebounce::kDelaySec - 1);
    EXPECT_EQ(r.above, 0);
    r.feed(Zone::Above, 1);
    EXPECT_EQ(r.above, 1);
    r.feed(Zone::In, ZoneDebounce::kDelaySec - 1);
    EXPECT_EQ(r.backIn, 0);
    r.feed(Zone::In, 1);
    EXPECT_EQ(r.backIn, 1);
}

TEST(ZoneDebounce, UnknownAndNoSignalPassAtOnceAndTheFirstReadingIsTakenAtOnce)
{
    ZoneDebounce d;
    EXPECT_EQ(d.tick(Zone::Unknown), Zone::Unknown);
    EXPECT_EQ(d.tick(Zone::Below), Zone::Below);      // first reading: no delay
    EXPECT_EQ(d.tick(Zone::NoSignal), Zone::NoSignal);
    EXPECT_EQ(d.tick(Zone::In), Zone::In);
    for (uint32_t i = 1; i < ZoneDebounce::kDelaySec; ++i) {
        EXPECT_EQ(d.tick(Zone::Above), Zone::In);
    }
    EXPECT_EQ(d.tick(Zone::Above), Zone::Above);
}
