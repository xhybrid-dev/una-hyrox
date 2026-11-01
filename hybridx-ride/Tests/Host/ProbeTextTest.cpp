// The bike probe's lines (ProbeText).

#include <gtest/gtest.h>

#include <cstring>

#include "ProbeText.hpp"

using namespace RideProbe;

TEST(ProbeText, TheStepsStartWithTheControlAndCoverTheReservedBits)
{
    ASSERT_EQ(kStepCount, 5);
    EXPECT_EQ(kSteps[0].kinds, 0x1u);   // SDK::Accessory::Kind::HRM
    EXPECT_EQ(kSteps[1].kinds, 0x2u);   // reserved: CADENCE = 1u << 1
    EXPECT_EQ(kSteps[2].kinds, 0x4u);   // reserved: POWER = 1u << 2
    for (const Step& s : kSteps) {
        EXPECT_LE(std::strlen(s.label), 18u);
    }
}

TEST(ProbeText, StatusLinesNameTheState)
{
    char b[48];
    statusLine(b, sizeof(b), 12345, 0x2, 2, "");
    EXPECT_STREQ(b, "12.3s 0x2 SEARCHING");
    statusLine(b, sizeof(b), 800, 0x1, 4, "Polar H10");
    EXPECT_STREQ(b, "0.8s 0x1 CONNECTED Polar H10");
    statusLine(b, sizeof(b), 0, 0x4, 9, nullptr);
    EXPECT_STREQ(b, "0.0s 0x4 state 9");
    char tiny[8];
    EXPECT_EQ(statusLine(tiny, sizeof(tiny), 12345, 0x2, 2, "x"), 7u);
    EXPECT_EQ(statusLine(nullptr, 4, 0, 0, 0, ""), 0u);
}

TEST(ProbeText, CsvRowsStayRows)
{
    char b[128];
    csvRow(b, sizeof(b), 1500, "status", 0x6, 0x2, 0, "KICKR, \"v5\"", 0, 0);
    EXPECT_STREQ(b, "1500,status,0x6,0x2,0,\"KICKR_ _v5_\",0,0\n");
    csvRow(b, sizeof(b), 2000, "hr", 0x1, 0, -1, "", 2, 143);
    EXPECT_STREQ(b, "2000,hr,0x1,0x0,-1,\"\",2,143\n");
}
