// HybridX Ride: auto-pause (AutoPause.hpp), the SDK Cycling example's detector.

#include <gtest/gtest.h>

#include "AutoPause.hpp"

using namespace Ride;
using Action = AutoPause::Action;
using St     = AutoPause::RideState;

TEST(AutoPause, NothingBeforeTheFirstFix)
{
    // A ride started in a car park before the GNSS has a fix: no samples, never paused.
    AutoPause a;
    a.reset();
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(a.tick(false, 0, St::Running), Action::None) << i;
    }
}

TEST(AutoPause, TwoSlowSamplesPauseAndTwoQuickOnesResume)
{
    AutoPause a;
    a.reset();
    EXPECT_EQ(a.tick(true, 6000, St::Running), Action::None);   // 21.6 km/h
    EXPECT_EQ(a.tick(true, 300, St::Running), Action::None);    // stopping: one slow sample
    EXPECT_EQ(a.tick(true, 100, St::Running), Action::Pause);   // the second
    EXPECT_EQ(a.tick(true, 700, St::AutoPaused), Action::None); // 2.5 km/h: between the thresholds
    EXPECT_EQ(a.tick(true, 1000, St::AutoPaused), Action::None);
    EXPECT_EQ(a.tick(true, 1500, St::AutoPaused), Action::Resume);
}

TEST(AutoPause, TheThresholdsAreStrict)
{
    AutoPause a;
    a.reset();
    EXPECT_EQ(a.tick(true, 500, St::Running), Action::None);    // 0.5 m/s is not below 0.5
    EXPECT_EQ(a.tick(true, 500, St::Running), Action::None);
    EXPECT_EQ(a.tick(true, 499, St::Running), Action::None);
    EXPECT_EQ(a.tick(true, 499, St::Running), Action::Pause);
    EXPECT_EQ(a.tick(true, 900, St::AutoPaused), Action::None); // 0.9 m/s is not above 0.9
    EXPECT_EQ(a.tick(true, 900, St::AutoPaused), Action::None);
    EXPECT_EQ(a.tick(true, 901, St::AutoPaused), Action::None);
    EXPECT_EQ(a.tick(true, 901, St::AutoPaused), Action::Resume);
}

TEST(AutoPause, OneQuickSampleBreaksTheDwell)
{
    AutoPause a;
    a.reset();
    EXPECT_EQ(a.tick(true, 100, St::Running), Action::None);
    EXPECT_EQ(a.tick(true, 3000, St::Running), Action::None);   // moving again: start over
    EXPECT_EQ(a.tick(true, 100, St::Running), Action::None);
    EXPECT_EQ(a.tick(true, 100, St::Running), Action::Pause);
}

TEST(AutoPause, AGapHoldsTheEvidenceAndCountsSamplesNotTicks)
{
    AutoPause a;
    a.reset();
    EXPECT_EQ(a.tick(true, 100, St::Running), Action::None);    // one slow sample
    EXPECT_EQ(a.tick(false, 0, St::Running), Action::None);     // a tick with none: held
    EXPECT_EQ(a.tick(false, 0, St::Running), Action::None);
    EXPECT_EQ(a.tick(true, 100, St::Running), Action::Pause);   // the second sample
}

TEST(AutoPause, StaleSpeedHoldsTheState)
{
    // Into a tunnel while paused at a light: five seconds with no fix, then a fix
    // that says "moving". The stale spell cleared the evidence, so two fresh samples
    // are still needed to resume.
    AutoPause a;
    a.reset();
    a.tick(true, 100, St::Running);
    EXPECT_EQ(a.tick(true, 100, St::Running), Action::Pause);
    a.tick(true, 2000, St::AutoPaused);
    for (int i = 0; i < kAutoPauseStaleSec; ++i) {
        EXPECT_EQ(a.tick(false, 0, St::AutoPaused), Action::None) << i;
    }
    EXPECT_EQ(a.tick(true, 2000, St::AutoPaused), Action::None);
    EXPECT_EQ(a.tick(true, 2000, St::AutoPaused), Action::Resume);
}

TEST(AutoPause, TheRidersPauseWins)
{
    AutoPause a;
    a.reset();
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(a.tick(true, 5000, St::ManuallyPaused), Action::None) << i;   // never resumes it
        EXPECT_EQ(a.tick(true, 0, St::ManuallyPaused), Action::None) << i;
    }
}
