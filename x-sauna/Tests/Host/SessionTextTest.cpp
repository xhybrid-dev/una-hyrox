#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "SessionText.hpp"

using namespace Sauna;

namespace {

template <typename F>
std::string text(F f)
{
    char buf[32];
    f(buf, sizeof(buf));
    return buf;
}

Recovery done(int16_t drop)
{
    Recovery r;
    r.state = Recovery::State::Done;
    r.drop  = drop;
    return r;
}

} // namespace

TEST(SessionText, Clock)
{
    auto c = [](uint32_t ms) { return text([&](char *b, size_t n) { formatClock(b, n, ms); }); };
    EXPECT_EQ(c(0), "00:00");
    EXPECT_EQ(c(999), "00:00");
    EXPECT_EQ(c(61000), "01:01");
    EXPECT_EQ(c(59 * 60000 + 59000), "59:59");
    EXPECT_EQ(c(3600000), "1:00:00");
    EXPECT_EQ(c(3600000 + 125000), "1:02:05");
}

TEST(SessionText, ClockIsCappedNotWrapped)
{
    char buf[16];
    formatClock(buf, sizeof(buf), 0xFFFFFFFFu);
    EXPECT_STREQ(buf, "99:59:59");
}

TEST(SessionText, Bpm)
{
    auto b = [](uint16_t v) { return text([&](char *o, size_t n) { formatBpm(o, n, v); }); };
    EXPECT_EQ(b(0), "--");
    EXPECT_EQ(b(72), "72");
    EXPECT_EQ(b(65535), "65535");
}

TEST(SessionText, Recovery)
{
    auto r = [](const Recovery &v) { return text([&](char *o, size_t n) { formatRecovery(o, n, v); }); };
    EXPECT_EQ(r(done(23)), "-23");
    EXPECT_EQ(r(done(-4)), "+4");
    EXPECT_EQ(r(done(0)), "0");
    EXPECT_EQ(r(done(INT16_MIN)), "+32768");
    Recovery p;
    p.state = Recovery::State::Pending;
    EXPECT_EQ(r(p), "..");
    Recovery n;
    n.state = Recovery::State::NoResult;
    EXPECT_EQ(r(n), "--");
    EXPECT_EQ(r(Recovery()), "--");
}

TEST(SessionText, RelativeTemperatureIsAlwaysSigned)
{
    auto t = [](int32_t v) { return text([&](char *o, size_t n) { formatRelTemp(o, n, v); }); };
    EXPECT_EQ(t(320), "+3.2");
    EXPECT_EQ(t(-50), "-0.5");
    EXPECT_EQ(t(0), "0.0");
    EXPECT_EQ(t(4), "0.0");        // rounds to nothing: no sign
    EXPECT_EQ(t(-4), "0.0");
    EXPECT_EQ(t(kX100Invalid), "--");
    EXPECT_EQ(t(12345), "+123.5");
}

TEST(SessionText, EveryWriterCutsShortAndNeverPastTheBuffer)
{
    for (size_t cap = 1; cap <= 12; ++cap) {
        for (int which = 0; which < 4; ++which) {
            char region[40];
            std::memset(region, 'G', sizeof(region));
            switch (which) {
            case 0: formatClock(region, cap, 3600000u + 125000u); break;
            case 1: formatBpm(region, cap, 65535); break;
            case 2: formatRecovery(region, cap, done(INT16_MIN)); break;
            default: formatRelTemp(region, cap, 12345); break;
            }
            for (size_t i = cap; i < sizeof(region); ++i) {
                ASSERT_EQ(region[i], 'G') << "writer " << which << " cap " << cap << " index " << i;
            }
            ASSERT_LT(std::strlen(region), cap) << "writer " << which << " cap " << cap;
        }
    }
}

TEST(SessionText, ZeroCapacityAndNullWriteNothing)
{
    char c = 'x';
    EXPECT_EQ(formatClock(&c, 0, 1000), 0u);
    EXPECT_EQ(formatBpm(nullptr, 8, 70), 0u);
    EXPECT_EQ(formatRelTemp(&c, 0, 100), 0u);
    EXPECT_EQ(formatRecovery(nullptr, 8, Recovery()), 0u);
    EXPECT_EQ(c, 'x');
}
