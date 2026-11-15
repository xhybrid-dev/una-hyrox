#include "TimeText.hpp"

#include <gtest/gtest.h>

#include <string>

using namespace XTraining;

TEST(TimeText, MinSec)
{
    EXPECT_EQ(TimeText::parseMinSec("22:00"), 1320u);
    EXPECT_EQ(TimeText::parseMinSec("9:59"), 599u);
    EXPECT_EQ(TimeText::parseMinSec("59:59"), 3599u);
    for (const char* bad : {"", "22", "22:0", "22:000", "22:60", "60:00", "122:00", ":30", "22:3a",
                            " 22:00", "22:00 ", "-1:00", "22.00"}) {
        EXPECT_EQ(TimeText::parseMinSec(bad), 0u) << "'" << bad << "'";
    }
    EXPECT_EQ(TimeText::parseMinSec(nullptr), 0u);
}

TEST(TimeText, Hms)
{
    EXPECT_EQ(TimeText::parseHms("1:45:00"), 6300u);
    EXPECT_EQ(TimeText::parseHms("0:55:00"), 3300u);
    for (const char* bad : {"", "1:45", "1:5:00", "1:45:0", "10:00:00", "1:60:00", "1:45:60",
                            "1:45:00x", "01:45:00"}) {
        EXPECT_EQ(TimeText::parseHms(bad), 0u) << "'" << bad << "'";
    }
}

TEST(TimeText, Date)
{
    EXPECT_EQ(TimeText::parseDate("2027-02-07"), 20270207u);
    EXPECT_EQ(TimeText::parseDate("2028-02-29"), 20280229u);
    for (const char* bad : {"", "2027-2-07", "2027-02-7", "2027/02/07", "2027-02-30", "2027-13-01",
                            "1999-12-31", "2027-02-07x", "20270207"}) {
        EXPECT_EQ(TimeText::parseDate(bad), 0u) << "'" << bad << "'";
    }
}

TEST(TimeText, Formatting)
{
    char buf[16];
    TimeText::duration(258, buf, sizeof(buf));
    EXPECT_STREQ(buf, "4:18");
    TimeText::duration(6067, buf, sizeof(buf));
    EXPECT_STREQ(buf, "1:41:07");
    TimeText::duration(0, buf, sizeof(buf));
    EXPECT_STREQ(buf, "0:00");
    TimeText::date(20270207u, buf, sizeof(buf));
    EXPECT_STREQ(buf, "2027-02-07");
    // Never past the buffer.
    char small[4];
    TimeText::duration(6067, small, sizeof(small));
    EXPECT_EQ(std::string(small).size(), 3u);
}
