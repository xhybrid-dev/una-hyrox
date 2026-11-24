// Floats as scaled integers and fixed-point text (no %f).
#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "Fixed.hpp"

using namespace EnergyProbe;

namespace
{
std::string fx(float v, uint8_t d)
{
    char buf[32];
    formatFixed(buf, sizeof(buf), v, d);
    return buf;
}
} // namespace

TEST(Fixed, Scaled)
{
    EXPECT_EQ(scaled(1.25f, 100), 125);
    EXPECT_EQ(scaled(-1.25f, 10), -13);      // half away from zero
    EXPECT_EQ(scaled(0.0f, 1000), 0);
    EXPECT_EQ(scaled(NAN, 10), kNoValue);
    EXPECT_EQ(scaled(INFINITY, 10), kNoValue);
    EXPECT_EQ(scaled(1e20f, 10), kNoValue);
}

TEST(Fixed, Text)
{
    EXPECT_EQ(fx(12.345f, 3), "12.345");
    EXPECT_EQ(fx(-0.5f, 2), "-0.50");
    EXPECT_EQ(fx(-0.004f, 2), "0.00");
    EXPECT_EQ(fx(7.0f, 0), "7");
    EXPECT_EQ(fx(3.14159f, 9), "3.141590");   // at most 6 places
    EXPECT_EQ(fx(NAN, 2), "nan");
    EXPECT_EQ(fx(1e12f, 3), "nan");           // beyond 32 bits of whole part
    EXPECT_EQ(fx(4000000000.0f, 0), "4000000000");
}

TEST(Fixed, SmallBuffers)
{
    char buf[4];
    formatFixed(buf, sizeof(buf), 123.456f, 3);
    EXPECT_EQ(std::string(buf), "123");   // snprintf truncation, still terminated
    EXPECT_EQ(formatFixed(nullptr, 10, 1.0f, 1), 0);
}
