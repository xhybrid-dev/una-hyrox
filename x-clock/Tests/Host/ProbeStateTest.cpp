#include <gtest/gtest.h>

#include <cstring>

#include "ProbeState.hpp"

using namespace ClockProbe;

TEST(ProbeState, RoundTrips)
{
    ProbeState s;
    s.starts   = 7;
    s.firstUtc = 1795000000;
    char buf[64];
    const size_t n = formatState(s, buf, sizeof(buf));
    EXPECT_STREQ(buf, "starts=7\nfirst=1795000000\n");
    ProbeState r;
    EXPECT_TRUE(parseState(buf, n, r));
    EXPECT_EQ(r.starts, 7u);
    EXPECT_EQ(r.firstUtc, 1795000000u);
}

TEST(ProbeState, AMissingOrJunkFileReadsAsTheDefaultAndSaysSo)
{
    ProbeState r;
    r.starts = 99;
    EXPECT_FALSE(parseState("", 0, r));
    EXPECT_EQ(r.starts, 0u);
    EXPECT_FALSE(parseState("hello\nworld=3\nstarts=\n", 21, r));
    EXPECT_EQ(r.starts, 0u);
    EXPECT_FALSE(parseState(nullptr, 0, r));
}

TEST(ProbeState, UnknownLinesAreIgnoredAndHugeNumbersClamp)
{
    const char* t = "future=1\nstarts=99999999999999\nfirst=5\n";
    ProbeState r;
    EXPECT_TRUE(parseState(t, std::strlen(t), r));
    EXPECT_EQ(r.starts, 0xFFFFFFFFu);
    EXPECT_EQ(r.firstUtc, 5u);
}

TEST(ProbeState, AShortBufferIsNeverOverrun)
{
    ProbeState s;
    s.starts = 123456;
    char buf[6];
    const size_t n = formatState(s, buf, sizeof(buf));
    EXPECT_LT(n, sizeof(buf));
    EXPECT_EQ(buf[n], '\0');
}
