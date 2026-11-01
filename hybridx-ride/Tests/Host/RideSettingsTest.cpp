// HybridX Ride: the settings file (RideSettings).

#include <gtest/gtest.h>

#include <cstring>

#include "RideSettings.hpp"

using Ride::RideSettings;

TEST(RideSettings, RoundTrips)
{
    RideSettings s;
    s.autoLapMin = 15;
    char buf[RideSettings::kMaxJsonBytes];
    const size_t len = s.toJson(buf, sizeof(buf));
    ASSERT_GT(len, 0u);
    EXPECT_STREQ(buf, R"({"v":1,"autolap":15,"outdoor":1})");
    RideSettings r;
    ASSERT_TRUE(r.fromJson(buf, len));
    EXPECT_EQ(r.autoLapMin, 15);
}

TEST(RideSettings, ABadFileKeepsTheDefaults)
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
