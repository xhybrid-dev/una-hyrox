// HybridX Workout: the settings file (WorkoutSettings).

#include <gtest/gtest.h>

#include <cstring>

#include "WorkoutSettings.hpp"

using namespace Workout;

TEST(WorkoutSettings, RoundTrips)
{
    WorkoutSettings s;
    s.autoLapMin = 15;
    char buf[WorkoutSettings::kMaxJsonBytes];
    const size_t len = s.toJson(buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    EXPECT_STREQ(buf, R"({"v":2,"autolap":15,"type":0,"sets":3,"srest":60,"scap":300,)"
                      R"("emom":10,"amrap":12,"work":40,"rest":20,"rounds":6,"crounds":8,"run":1000})");
    WorkoutSettings r;
    ASSERT_TRUE(r.fromJson(buf, len));
    EXPECT_EQ(r.autoLapMin, 15);
    EXPECT_EQ(r.plan.type, SessionType::Plain);
}

TEST(WorkoutSettings, EveryPlanFieldRoundTrips)
{
    WorkoutSettings s;
    s.plan.type          = SessionType::Custom;
    s.plan.sets          = 5;
    s.plan.setRestSec    = 90;
    s.plan.setCapSec     = 120;
    s.plan.emomMin       = 20;
    s.plan.amrapMin      = 15;
    s.plan.workSec       = 30;
    s.plan.restSec       = 15;
    s.plan.rounds        = 10;
    s.plan.circuitRounds = 4;
    s.plan.runM          = 500;
    char buf[WorkoutSettings::kMaxJsonBytes];
    const size_t len = s.toJson(buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    WorkoutSettings r;
    ASSERT_TRUE(r.fromJson(buf, len));
    EXPECT_EQ(r.plan.type, SessionType::Custom);
    EXPECT_EQ(r.plan.sets, 5);
    EXPECT_EQ(r.plan.setRestSec, 90);
    EXPECT_EQ(r.plan.setCapSec, 120);
    EXPECT_EQ(r.plan.emomMin, 20);
    EXPECT_EQ(r.plan.amrapMin, 15);
    EXPECT_EQ(r.plan.workSec, 30);
    EXPECT_EQ(r.plan.restSec, 15);
    EXPECT_EQ(r.plan.rounds, 10);
    EXPECT_EQ(r.plan.circuitRounds, 4);
    EXPECT_EQ(r.plan.runM, 500);
}

TEST(WorkoutSettings, AVersionOneFileKeepsItsAutoLapAndTakesThePlanDefaults)
{
    const char *w0 = R"({"v":1,"autolap":5})";
    WorkoutSettings s;
    ASSERT_TRUE(s.fromJson(w0, std::strlen(w0)));
    EXPECT_EQ(s.autoLapMin, 5);
    EXPECT_EQ(s.plan.type, SessionType::Plain);
    EXPECT_EQ(s.plan.sets, kSetsDefault);
    EXPECT_EQ(s.plan.runM, kRunMDefault);
}

TEST(WorkoutSettings, OutOfRangeValuesArePulledIntoRange)
{
    const char *t = R"({"v":2,"autolap":0,"type":99,"sets":200,"srest":1,"scap":99999,)"
                    R"("emom":0,"amrap":-3,"work":1,"rest":70000,"rounds":31,"crounds":9,"run":1050})";
    WorkoutSettings s;
    ASSERT_TRUE(s.fromJson(t, std::strlen(t)));
    EXPECT_EQ(s.plan.type, SessionType::Plain);
    EXPECT_EQ(s.plan.sets, kSetsMax);
    EXPECT_EQ(s.plan.setRestSec, kSetRestSecMin);
    EXPECT_EQ(s.plan.setCapSec, kSetCapSecMax);
    EXPECT_EQ(s.plan.emomMin, kEmomMinMin);
    EXPECT_EQ(s.plan.amrapMin, kAmrapMinMin);
    EXPECT_EQ(s.plan.workSec, kWorkSecMin);
    EXPECT_EQ(s.plan.restSec, kRestSecMax);
    EXPECT_EQ(s.plan.rounds, kRoundsMax);
    EXPECT_EQ(s.plan.circuitRounds, kCircuitRoundsMax);
    EXPECT_EQ(s.plan.runM, 1000);
}

TEST(WorkoutSettings, MissingPlanKeysKeepTheirDefaults)
{
    const char *t = R"({"v":2,"autolap":10,"type":2})";
    WorkoutSettings s;
    ASSERT_TRUE(s.fromJson(t, std::strlen(t)));
    EXPECT_EQ(s.autoLapMin, 10);
    EXPECT_EQ(s.plan.type, SessionType::Emom);
    EXPECT_EQ(s.plan.emomMin, kEmomMinDefault);
}

TEST(WorkoutSettings, ABadFileKeepsTheDefaults)
{
    const char* bad[] = {
        "",
        "not json",
        R"({"v":3,"autolap":5})",      // a newer version
        R"({"v":2,"autolap":7})",      // not one of the choices
        R"({"v":1,"autolap":7})",      // not one of the choices
        R"({"v":1,"autolap":-5})",
        R"({"v":1})",
    };
    for (const char* t : bad) {
        WorkoutSettings s;
        EXPECT_FALSE(s.fromJson(t, std::strlen(t))) << t;
        EXPECT_EQ(s.autoLapMin, 0) << t;
        EXPECT_EQ(s.plan.type, SessionType::Plain) << t;
    }
    WorkoutSettings s;
    EXPECT_FALSE(s.fromJson(nullptr, 4));
}

TEST(WorkoutSettings, ATooSmallBufferWritesNothing)
{
    WorkoutSettings s;
    s.autoLapMin = 30;
    char tiny[8];
    EXPECT_EQ(s.toJson(tiny, sizeof(tiny)), 0u);
    EXPECT_STREQ(tiny, "");
}
