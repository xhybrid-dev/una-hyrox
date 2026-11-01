// Which barometer readings are used: a NaN must never reach the filter or the climb counter.
#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "RideAltitude.hpp"

using namespace Ride;

namespace {
constexpr float kNan = std::numeric_limits<float>::quiet_NaN();
constexpr float kInf = std::numeric_limits<float>::infinity();
} // namespace

TEST(RideAltitude, PressuresMustBeRealAndPositive)
{
    EXPECT_TRUE(usablePressurePa(101325.0f));
    EXPECT_TRUE(usablePressurePa(54000.0f));      // high in the mountains
    EXPECT_FALSE(usablePressurePa(0.0f));
    EXPECT_FALSE(usablePressurePa(-1.0f));
    EXPECT_FALSE(usablePressurePa(kNan));         // what the SDK's getP0() gives with no data
    EXPECT_FALSE(usablePressurePa(kInf));
}

TEST(RideAltitude, AltitudesMustBeRealAndPhysical)
{
    EXPECT_TRUE(usableAltitudeM(0.0f));
    EXPECT_TRUE(usableAltitudeM(-400.0f));
    EXPECT_TRUE(usableAltitudeM(2500.0f));
    EXPECT_TRUE(usableAltitudeM(kMinAltitudeM));
    EXPECT_TRUE(usableAltitudeM(kMaxAltitudeM));
    EXPECT_FALSE(usableAltitudeM(kMinAltitudeM - 0.1f));
    EXPECT_FALSE(usableAltitudeM(kMaxAltitudeM + 0.1f));
    EXPECT_FALSE(usableAltitudeM(kNan));          // what the SDK's getAltitude() gives for a bad input
    EXPECT_FALSE(usableAltitudeM(kInf));
    EXPECT_FALSE(usableAltitudeM(-kInf));
}

TEST(RideAltitude, TheSdksOwnFormulaGivesNanForABadSeaLevelAndTheGuardCatchesIt)
{
    // The SDK's ISA formula (Pressure::getAltitude), reproduced here to show the failure the guard is for.
    auto isa = [](float p, float p0) {
        if (!(p > 0.0f) || !(p0 > 0.0f)) {
            return kNan;
        }
        return 44330.0f * (1.0f - std::pow(p / p0, 1.0f / 5.255877f));   // the SDK's constants
    };
    EXPECT_TRUE(usableAltitudeM(isa(101325.0f, 101325.0f)));
    EXPECT_NEAR(isa(100000.0f, 101325.0f), 110.9f, 0.3f);       // 13.25 hPa lower: about 111 m up (8.4 m per hPa)
    EXPECT_FALSE(usableAltitudeM(isa(100000.0f, kNan)));
    EXPECT_FALSE(usableAltitudeM(isa(kNan, 101325.0f)));
}
