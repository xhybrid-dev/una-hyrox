// The GPS side of a ride: distance, climb and speed in the model, in the FIT values, in the text,
// and the Indoor/Outdoor switch in the settings file. Figures are worked by hand in the comments.
#include <gtest/gtest.h>

#include <cstring>

#include "RideFit.hpp"
#include "RideModel.hpp"
#include "RideSettings.hpp"
#include "RideText.hpp"

using namespace Ride;

namespace {

RideConfig cfg(bool outdoor)
{
    RideConfig c;
    c.outdoor = outdoor;
    return c;
}

Motion motion(uint32_t distCm, uint32_t ascentCm, uint32_t descentCm, uint32_t speedMmS)
{
    Motion m;
    m.distCm    = distCm;
    m.ascentCm  = ascentCm;
    m.descentCm = descentCm;
    m.speedMmS  = speedMmS;
    return m;
}

std::string text(size_t (*f)(char *, size_t, uint32_t, bool), uint32_t v, bool imperial)
{
    char buf[24];
    f(buf, sizeof(buf), v, imperial);
    return buf;
}

// Both a plain start and one just before the 32-bit clock wraps.
constexpr uint32_t kStarts[] = {0u, 123456u, 0xFFFFFFFFu - 90000u};

} // namespace

TEST(RideMotion, OutdoorAddsDistanceClimbAndTopSpeed)
{
    for (uint32_t s : kStarts) {
        RideModel m;
        m.start(s, cfg(true));
        m.addSample(s + 1000, 120, motion(500, 100, 0, 5000));      // 5 m, climbed 1 m
        m.addSample(s + 2000, 121, motion(700, 0, 200, 7000));      // 7 m, dropped 2 m
        EXPECT_EQ(m.distCm(), 1200u);
        EXPECT_EQ(m.ascentCm(), 100u);
        EXPECT_EQ(m.descentCm(), 200u);

        const SessionSummary sum = m.summary();
        EXPECT_EQ(sum.distCm, 1200u);
        EXPECT_EQ(sum.maxSpeedMmS, 7000u);
        EXPECT_EQ(sum.avgSpeedMmS, 6000u);          // 12 m in 2 s of active time
        EXPECT_EQ(m.current().distCm, 1200u);
    }
}

TEST(RideMotion, IndoorIgnoresMotion)
{
    RideModel m;
    m.start(0, cfg(false));
    m.addSample(1000, 120, motion(500, 100, 100, 5000));
    EXPECT_EQ(m.distCm(), 0u);
    EXPECT_EQ(m.summary().maxSpeedMmS, 0u);
    EXPECT_EQ(m.summary().avgSpeedMmS, 0u);
}

TEST(RideMotion, APauseAddsNoDistance)
{
    RideModel m;
    m.start(0, cfg(true));
    m.addSample(1000, 120, motion(500, 0, 0, 5000));
    EXPECT_TRUE(m.pause(1500));
    m.addSample(2000, 120, motion(900, 0, 0, 9000));       // refused: not running
    EXPECT_EQ(m.distCm(), 500u);
    EXPECT_EQ(m.summary().maxSpeedMmS, 5000u);
}

TEST(RideMotion, LapsKeepTheirOwnMotionAndAddUpToTheRide)
{
    RideModel m;
    m.start(0, cfg(true));
    m.addSample(1000, 120, motion(500, 100, 0, 5000));
    EXPECT_EQ(m.lap(1500), LapResult::Ok);
    m.addSample(2500, 120, motion(300, 0, 50, 3000));
    EXPECT_EQ(m.lapAt(0).distCm, 500u);
    EXPECT_EQ(m.lapAt(0).maxSpeedMmS, 5000u);
    EXPECT_EQ(m.lapAt(1).distCm, 300u);
    EXPECT_EQ(m.lapAt(1).descentCm, 50u);
    EXPECT_EQ(m.lapAt(1).maxSpeedMmS, 3000u);
    EXPECT_EQ(m.distCm(), 800u);
    // Lap 2 held 1.0 s of active time (1500 to 2500): 3 m in 1 s.
    EXPECT_EQ(m.lapAt(1).avgSpeedMmS(), 3000u);
}

TEST(RideMotion, TotalsSaturateInsteadOfWrapping)
{
    RideModel m;
    m.start(0, cfg(true));
    m.addSample(1000, 0, motion(0xFFFFFFF0u, 0, 0, 0));
    m.addSample(2000, 0, motion(100, 0, 0, 0));
    EXPECT_EQ(m.distCm(), 0xFFFFFFFFu);
    EXPECT_GT(m.summary().avgSpeedMmS, 0u);        // the 64-bit sum does not overflow
}

TEST(RideMotion, FitValuesCarryDistanceSpeedAndWholeMetresOfClimb)
{
    RideModel m;
    m.start(0, cfg(true));
    m.addSample(1000, 120, motion(2000, 149, 150, 4000));
    m.lap(1500);
    m.addSample(2500, 130, motion(3000, 0, 0, 3000));
    m.stop(2500);

    FitLap laps[4];
    FitSession ses;
    const uint8_t n = buildFit(m, laps, 4, ses);
    ASSERT_EQ(n, 2);
    EXPECT_EQ(laps[0].distCm, 2000u);
    EXPECT_EQ(laps[0].maxSpeedMmS, 4000u);
    EXPECT_EQ(laps[0].ascentM, 1u);                // 149 cm rounds down
    EXPECT_EQ(laps[0].descentM, 2u);               // 150 cm rounds up
    EXPECT_EQ(ses.distCm, 5000u);
    EXPECT_EQ(ses.maxSpeedMmS, 4000u);
    EXPECT_EQ(ses.avgSpeedMmS, 20000u);            // 50 m in 2.5 s of active time
    EXPECT_EQ(ses.ascentM, 1u);
}

TEST(RideMotion, ClimbCapsBelowTheInvalidValue)
{
    RideModel m;
    m.start(0, cfg(true));
    m.addSample(1000, 0, motion(0, 0xFFFFFFF0u, 0, 0));
    m.stop(1000);
    FitLap laps[1];
    FitSession ses;
    buildFit(m, laps, 1, ses);
    EXPECT_EQ(ses.ascentM, 65534u);
}

TEST(RideText, Speed)
{
    EXPECT_EQ(text(formatSpeed, 0, false), "0.0");
    EXPECT_EQ(text(formatSpeed, 10000, false), "36.0");       // 10 m/s
    EXPECT_EQ(text(formatSpeed, 7611, false), "27.4");        // 27.40 km/h
    EXPECT_EQ(text(formatSpeed, 10000, true), "22.4");        // 22.37 mph
    EXPECT_EQ(text(formatSpeed, 0xFFFFFFFFu, false), "999.9"); // capped
}

TEST(RideText, Distance)
{
    EXPECT_EQ(text(formatDistance, 0, false), "0.00");
    EXPECT_EQ(text(formatDistance, 100000, false), "1.00");
    EXPECT_EQ(text(formatDistance, 123400, false), "1.23");
    EXPECT_EQ(text(formatDistance, 123500, false), "1.24");    // rounds half up
    EXPECT_EQ(text(formatDistance, 160934, true), "1.00");     // one mile
    EXPECT_EQ(text(formatDistance, 5000000, false), "50.00");
    EXPECT_EQ(text(formatDistance, 99999, false), "1.00");             // 999.99 m
    EXPECT_EQ(text(formatDistance, 99999000, false), "999.99");        // the largest shown
    EXPECT_EQ(text(formatDistance, 100000000, false), "999.99");       // 1000 km: capped
    EXPECT_EQ(text(formatDistance, 0xFFFFFFFFu, false), "999.99");
    EXPECT_EQ(text(formatDistance, 0xFFFFFFFFu, true), "999.99");
}

TEST(RideText, Climb)
{
    EXPECT_EQ(text(formatClimb, 0, false), "0");
    EXPECT_EQ(text(formatClimb, 12345, false), "123");
    EXPECT_EQ(text(formatClimb, 12350, false), "124");
    EXPECT_EQ(text(formatClimb, 30480, true), "1000");         // 1000 ft
    EXPECT_EQ(text(formatClimb, 0xFFFFFFFFu, false), "99999");
}

TEST(RideText, TinyBuffersAreSafe)
{
    char buf[4];
    formatSpeed(buf, sizeof(buf), 10000, false);
    EXPECT_STREQ(buf, "36.");
    formatDistance(buf, 1, 100000, false);
    EXPECT_STREQ(buf, "");
    EXPECT_EQ(formatClimb(nullptr, 4, 100, false), 0u);
    EXPECT_EQ(formatClimb(buf, 0, 100, false), 0u);
}

TEST(RideSettings, ModeRoundTripsAndDefaultsToOutdoor)
{
    RideSettings d;
    EXPECT_TRUE(d.outdoor);

    RideSettings s;
    s.autoLapMin = 5;
    s.outdoor    = false;
    char buf[RideSettings::kMaxJsonBytes];
    const size_t n = s.toJson(buf, sizeof(buf));
    ASSERT_GT(n, 0u);

    RideSettings r;
    ASSERT_TRUE(r.fromJson(buf, n));
    EXPECT_EQ(r.autoLapMin, 5);
    EXPECT_FALSE(r.outdoor);
}

TEST(RideSettings, AFileWithoutTheModeKeepsTheDefault)
{
    const char *j = "{\"v\":1,\"autolap\":10}";
    RideSettings r;
    ASSERT_TRUE(r.fromJson(j, std::strlen(j)));
    EXPECT_EQ(r.autoLapMin, 10);
    EXPECT_TRUE(r.outdoor);
}

TEST(RideSettings, ABadModeIsRefused)
{
    const char *j = "{\"v\":1,\"autolap\":10,\"outdoor\":2}";
    RideSettings r;
    EXPECT_FALSE(r.fromJson(j, std::strlen(j)));
    EXPECT_EQ(r.autoLapMin, 0);
    EXPECT_TRUE(r.outdoor);
}
