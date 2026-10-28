// HybridX Workout: the settings file (WorkoutSettings).

#include <gtest/gtest.h>

#include <cstring>

#include "WorkoutSettings.hpp"

using Workout::WorkoutSettings;

TEST(WorkoutSettings, RoundTrips)
{
    WorkoutSettings s;
    s.autoLapMin = 15;
    char buf[WorkoutSettings::kMaxJsonBytes];
    const size_t len = s.toJson(buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    EXPECT_STREQ(buf, R"({"v":1,"autolap":15})");
    WorkoutSettings r;
    ASSERT_TRUE(r.fromJson(buf, len));
    EXPECT_EQ(r.autoLapMin, 15);
}

TEST(WorkoutSettings, ABadFileKeepsTheDefaults)
{
    const char* bad[] = {
        "",
        "not json",
        R"({"v":2,"autolap":5})",      // a newer version
        R"({"v":1,"autolap":7})",      // not one of the choices
        R"({"v":1,"autolap":-5})",
        R"({"v":1})",
    };
    for (const char* t : bad) {
        WorkoutSettings s;
        EXPECT_FALSE(s.fromJson(t, std::strlen(t))) << t;
        EXPECT_EQ(s.autoLapMin, 0) << t;
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
