// HybridX Ride: the settings file (RideSettings).

#include <gtest/gtest.h>

#include <cstring>

#include "RideSettings.hpp"

using namespace Ride;

TEST(RideSettings, RoundTrips)
{
    RideSettings s;
    s.autoLapMin = 15;
    char buf[RideSettings::kMaxJsonBytes];
    const size_t len = s.toJson(buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    EXPECT_STREQ(buf, R"({"v":2,"autolap":15,"outdoor":1,"autopause":1,"lapkm":0,"type":0,"work":40,"rest":20,"rounds":6})");
    RideSettings r;
    ASSERT_TRUE(r.fromJson(buf, len));
    EXPECT_EQ(r.autoLapMin, 15);
}

TEST(RideSettings, VersionTwoRoundTrips)
{
    RideSettings s;
    s.outdoor      = false;
    s.autoPause    = false;
    s.autoLapKm    = 5;
    s.plan.type    = SessionType::Custom;
    s.plan.workSec = 120;
    s.plan.restSec = 60;
    s.plan.rounds  = 8;
    char buf[RideSettings::kMaxJsonBytes];
    const size_t len = s.toJson(buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    RideSettings r;
    ASSERT_TRUE(r.fromJson(buf, len));
    EXPECT_FALSE(r.outdoor);
    EXPECT_FALSE(r.autoPause);
    EXPECT_EQ(r.autoLapKm, 5);
    EXPECT_EQ(r.plan.type, SessionType::Custom);
    EXPECT_EQ(r.plan.workSec, 120);
    EXPECT_EQ(r.plan.restSec, 60);
    EXPECT_EQ(r.plan.rounds, 8);
}

TEST(RideSettings, AVersionOneFileTakesTheNewDefaults)
{
    const char *t = R"({"v":1,"autolap":5,"outdoor":0})";
    RideSettings s;
    ASSERT_TRUE(s.fromJson(t, std::strlen(t)));
    EXPECT_EQ(s.autoLapMin, 5);
    EXPECT_FALSE(s.outdoor);
    EXPECT_TRUE(s.autoPause);                // on by default
    EXPECT_EQ(s.autoLapKm, 0);
    EXPECT_EQ(s.plan.type, SessionType::Plain);
}

TEST(RideSettings, OneAutoLapAtATimeAndPlansClamped)
{
    const char *both = R"({"v":2,"autolap":5,"lapkm":10})";   // hand-edited: the time wins
    RideSettings a;
    ASSERT_TRUE(a.fromJson(both, std::strlen(both)));
    EXPECT_EQ(a.autoLapMin, 5);
    EXPECT_EQ(a.autoLapKm, 0);
    const char *wild = R"({"v":2,"autolap":0,"type":9,"work":1,"rest":9999,"rounds":0})";
    RideSettings b;
    ASSERT_TRUE(b.fromJson(wild, std::strlen(wild)));
    EXPECT_EQ(b.plan.type, SessionType::Plain);
    EXPECT_EQ(b.plan.workSec, kWorkSecMin);
    EXPECT_EQ(b.plan.restSec, kRestSecMax);
    EXPECT_EQ(b.plan.rounds, kRoundsMin);
    const char *badKm = R"({"v":2,"autolap":0,"lapkm":3})";   // not one of the choices
    RideSettings c;
    EXPECT_FALSE(c.fromJson(badKm, std::strlen(badKm)));
}

TEST(RideSettings, ABadFileKeepsTheDefaults)
{
    const char* bad[] = {
        "",
        "not json",
        R"({"v":3,"autolap":5})",      // a newer version
        R"({"v":1,"autolap":7})",      // not one of the choices
        R"({"v":1,"autolap":-5})",
        R"({"v":1})",
    };
    for (const char* t : bad) {
        RideSettings s;
        EXPECT_FALSE(s.fromJson(t, std::strlen(t))) << t;
        EXPECT_EQ(s.autoLapMin, 0) << t;
    }
    RideSettings s;
    EXPECT_FALSE(s.fromJson(nullptr, 4));
}

TEST(RideSettings, ATooSmallBufferWritesNothing)
{
    RideSettings s;
    s.autoLapMin = 30;
    char tiny[8];
    EXPECT_EQ(s.toJson(tiny, sizeof(tiny)), 0u);
    EXPECT_STREQ(tiny, "");
}
