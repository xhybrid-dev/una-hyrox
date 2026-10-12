#include "WeighLog.hpp"

#include "Calendar.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

using namespace XWeighIn;

namespace
{
Entry entry(int32_t y, uint32_t m, uint32_t d, uint16_t hh, uint16_t mm, int32_t hundredths)
{
    Entry e;
    e.day        = daysFromCivil(y, m, d);
    e.minute     = static_cast<uint16_t>(hh * 60 + mm);
    e.hundredths = hundredths;
    return e;
}

bool parse(const std::string& line, Entry& out)
{
    return parseLine(line.data(), line.size(), out);
}

bool rejects(const std::string& line)
{
    Entry e;
    return !parse(line, e);
}
}  // namespace

TEST(WeighLog, FormatsOneLine)
{
    char buf[kMaxLineBytes];
    const size_t n = formatEntry(entry(2026, 10, 12, 7, 31, 8230), buf, sizeof(buf));
    EXPECT_STREQ(buf, "2026-10-12,07:31,82.30");
    EXPECT_EQ(n, std::strlen("2026-10-12,07:31,82.30"));

    formatEntry(entry(2026, 1, 2, 0, 5, 6005), buf, sizeof(buf));
    EXPECT_STREQ(buf, "2026-01-02,00:05,60.05");
    formatEntry(entry(2026, 12, 31, 23, 59, 50000), buf, sizeof(buf));
    EXPECT_STREQ(buf, "2026-12-31,23:59,500.00");
}

TEST(WeighLog, RoundTrips)
{
    const Entry in = entry(2026, 10, 12, 7, 31, 8230);
    char buf[kMaxLineBytes];
    const size_t n = formatEntry(in, buf, sizeof(buf));
    Entry out;
    ASSERT_TRUE(parseLine(buf, n, out));
    EXPECT_EQ(out.day, in.day);
    EXPECT_EQ(out.minute, in.minute);
    EXPECT_EQ(out.hundredths, in.hundredths);
}

TEST(WeighLog, ZeroOneOrTwoDecimals)
{
    Entry e;
    ASSERT_TRUE(parse("2026-10-12,07:31,82", e));
    EXPECT_EQ(e.hundredths, 8200);
    ASSERT_TRUE(parse("2026-10-12,07:31,82.3", e));
    EXPECT_EQ(e.hundredths, 8230);
    ASSERT_TRUE(parse("2026-10-12,07:31,82.30", e));
    EXPECT_EQ(e.hundredths, 8230);
    ASSERT_TRUE(parse("2026-10-12,07:31,82.05", e));
    EXPECT_EQ(e.hundredths, 8205);
    ASSERT_TRUE(parse("2026-10-12,07:31,500.00", e));
    EXPECT_EQ(e.hundredths, 50000);
    ASSERT_TRUE(parse("2026-10-12,07:31,0.01", e));
    EXPECT_EQ(e.hundredths, 1);
}

TEST(WeighLog, WindowsLineEndingIsFine)
{
    Entry e;
    ASSERT_TRUE(parse("2026-10-12,07:31,82.30\r", e));
    EXPECT_EQ(e.hundredths, 8230);
}

TEST(WeighLog, RejectsWhatIsNotAnEntry)
{
    EXPECT_TRUE(rejects(kLogHeader));
    EXPECT_TRUE(rejects(""));
    EXPECT_TRUE(rejects("\r"));
    EXPECT_TRUE(rejects("2026-10-13,07:"));             // a torn line
    EXPECT_TRUE(rejects("2026-02-30,07:31,82.30"));     // no such day
    EXPECT_TRUE(rejects("1999-12-31,07:31,82.30"));     // outside the accepted years
    EXPECT_TRUE(rejects("2026-1-12,07:31,82.30"));      // short field
    EXPECT_TRUE(rejects("2026-10-12,24:00,82.30"));
    EXPECT_TRUE(rejects("2026-10-12,07:60,82.30"));
    EXPECT_TRUE(rejects("2026-10-12 07:31,82.30"));     // wrong separator
    EXPECT_TRUE(rejects("2026-10-12,07:31,0"));         // no weight
    EXPECT_TRUE(rejects("2026-10-12,07:31,0.00"));
    EXPECT_TRUE(rejects("2026-10-12,07:31,500.01"));    // above the maximum
    EXPECT_TRUE(rejects("2026-10-12,07:31,1234"));      // four whole digits
    EXPECT_TRUE(rejects("2026-10-12,07:31,82.305"));    // three decimals
    EXPECT_TRUE(rejects("2026-10-12,07:31,82."));
    EXPECT_TRUE(rejects("2026-10-12,07:31,.5"));
    EXPECT_TRUE(rejects("2026-10-12,07:31,-82.30"));
    EXPECT_TRUE(rejects("2026-10-12,07:31,82,30"));     // a decimal comma
    EXPECT_TRUE(rejects("2026-10-12,07:31,82.30,x"));   // an extra field
    EXPECT_TRUE(rejects("2026-10-12,07:31,82.30 "));
    EXPECT_TRUE(rejects(std::string(70, '1')));         // overlong
}

TEST(WeighLog, RejectionLeavesTheEntryUntouched)
{
    Entry e = entry(2026, 10, 12, 7, 31, 8230);
    EXPECT_FALSE(parse("junk", e));
    EXPECT_EQ(e.hundredths, 8230);
}

TEST(WeighLog, ValidEntries)
{
    EXPECT_TRUE(isValidEntry(entry(2026, 10, 12, 7, 31, 8230)));
    Entry e = entry(2026, 10, 12, 7, 31, 8230);
    e.minute = 1440;
    EXPECT_FALSE(isValidEntry(e));
    e = entry(2026, 10, 12, 7, 31, 0);
    EXPECT_FALSE(isValidEntry(e));
    e = entry(2026, 10, 12, 7, 31, 50001);
    EXPECT_FALSE(isValidEntry(e));
    e = entry(2026, 10, 12, 7, 31, 8230);
    e.day = firstValidDay() - 1;
    EXPECT_FALSE(isValidEntry(e));
}

TEST(WeighLog, FormatRefusesBadEntriesAndSmallBuffers)
{
    char buf[kMaxLineBytes];
    std::memset(buf, 'x', sizeof(buf));
    EXPECT_EQ(formatEntry(entry(2026, 10, 12, 7, 31, 0), buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "");

    char small[10];
    EXPECT_EQ(formatEntry(entry(2026, 10, 12, 7, 31, 8230), small, sizeof(small)), 0u);
    EXPECT_STREQ(small, "");

    EXPECT_EQ(formatEntry(entry(2026, 10, 12, 7, 31, 8230), nullptr, 32), 0u);
}
