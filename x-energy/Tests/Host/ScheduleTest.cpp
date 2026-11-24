// The Energy Probe's schedule: which night or day a time is in, and its mode.
#include <gtest/gtest.h>

#include "CivilDate.hpp"
#include "Schedule.hpp"

using namespace EnergyProbe;

namespace
{
const int32_t kDay = daysFromCivil(2026, 11, 24);   // a Tuesday

LocalTime at(int32_t day, uint8_t h, uint8_t m = 0)
{
    LocalTime t;
    t.day = day;
    t.hour = h;
    t.minute = m;
    return t;
}
} // namespace

TEST(CivilDate, RoundTrips)
{
    EXPECT_EQ(daysFromCivil(1970, 1, 1), 0);
    const CivilDate c = civilFromDays(kDay);
    EXPECT_EQ(c.year, 2026);
    EXPECT_EQ(c.month, 11);
    EXPECT_EQ(c.day, 24);
    EXPECT_EQ(yyyymmdd(kDay), 20261124u);
    EXPECT_EQ(yyyymmdd(daysFromCivil(2028, 2, 29)), 20280229u);
    EXPECT_STREQ(monthShort(11), "Nov");
    EXPECT_STREQ(monthShort(0), "?");
}

TEST(Schedule, WindowAcrossMidnight)
{
    const Window w = window(kDefaultWindow);   // 22:00-08:00
    EXPECT_EQ(w.startHour, 22);
    EXPECT_EQ(periodAt(at(kDay, 21, 59), w), (Period{false, kDay}));
    EXPECT_EQ(periodAt(at(kDay, 22, 0), w), (Period{true, kDay}));
    EXPECT_EQ(periodAt(at(kDay + 1, 0, 30), w), (Period{true, kDay}));     // the same night
    EXPECT_EQ(periodAt(at(kDay + 1, 7, 59), w), (Period{true, kDay}));
    EXPECT_EQ(periodAt(at(kDay + 1, 8, 0), w), (Period{false, kDay + 1}));
}

TEST(Schedule, WindowFromMidnight)
{
    const Window w = window(3);   // 00:00-10:00
    EXPECT_EQ(w.startHour, 0);
    EXPECT_EQ(periodAt(at(kDay, 23, 0), w), (Period{false, kDay}));
    EXPECT_EQ(periodAt(at(kDay + 1, 0, 0), w), (Period{true, kDay}));      // still the 24th's night
    EXPECT_EQ(periodAt(at(kDay + 1, 9, 59), w), (Period{true, kDay}));
    EXPECT_EQ(periodAt(at(kDay + 1, 10, 0), w), (Period{false, kDay + 1}));
    EXPECT_EQ(tonight(at(kDay, 15, 0), w), kDay);
}

TEST(Schedule, Tonight)
{
    const Window w = window(kDefaultWindow);
    EXPECT_EQ(tonight(at(kDay, 12, 0), w), kDay);
    EXPECT_EQ(tonight(at(kDay, 23, 0), w), kDay);
    EXPECT_EQ(tonight(at(kDay + 1, 3, 0), w), kDay);
    EXPECT_EQ(tonight(at(kDay + 1, 9, 0), w), kDay + 1);
}

TEST(Schedule, BadWindowIndexFallsBack)
{
    EXPECT_EQ(window(200).startHour, window(kDefaultWindow).startHour);
}

TEST(Schedule, RotationFromTheFirstNight)
{
    Settings s;
    // No first night yet: tonight is night 0, Full.
    EXPECT_EQ(rotationFor(kDay, s), Mode::Full);
    s.firstNight = kDay;
    const Mode expected[] = {Mode::Full, Mode::Off, Mode::Full, Mode::Off, Mode::Motion,
                             Mode::Heart, Mode::HeartSparse, Mode::Off, Mode::Full, Mode::Off};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(modeFor(Period{true, kDay + i}, s), expected[i]) << "night " << i;
    }
    // A clock set back before the first night reads as night 0.
    EXPECT_EQ(modeFor(Period{true, kDay - 3}, s), Mode::Full);
}

TEST(Schedule, DaysFollowTheDayMode)
{
    Settings s;
    s.firstNight = kDay;
    EXPECT_EQ(modeFor(Period{false, kDay + 1}, s), Mode::Full);
    s.dayMode = Mode::Off;
    EXPECT_EQ(modeFor(Period{false, kDay + 1}, s), Mode::Off);
    s.dayMode = Mode::Heart;   // only Full or Off for a day
    EXPECT_EQ(modeFor(Period{false, kDay + 1}, s), Mode::Full);
}

TEST(Schedule, OverrideIsForOneNight)
{
    Settings s;
    s.firstNight = kDay;
    s.overrideDay = kDay + 1;   // an Off night in the rotation
    s.overrideMode = 1 + static_cast<uint8_t>(Mode::Heart);
    EXPECT_EQ(modeFor(Period{true, kDay + 1}, s), Mode::Heart);
    EXPECT_EQ(modeFor(Period{true, kDay + 2}, s), Mode::Full);
    EXPECT_EQ(modeFor(Period{false, kDay + 1}, s), Mode::Full);   // not the day
    s.overrideMode = kOverrideAuto;
    EXPECT_EQ(modeFor(Period{true, kDay + 1}, s), Mode::Off);
    s.overrideMode = 99;   // nonsense: ignored
    EXPECT_EQ(modeFor(Period{true, kDay + 1}, s), Mode::Off);
}

TEST(Schedule, MinutesIntoPeriod)
{
    const Window w = window(kDefaultWindow);
    EXPECT_EQ(minutesIntoPeriod(at(kDay, 22, 0), w), 0u);
    EXPECT_EQ(minutesIntoPeriod(at(kDay, 23, 5), w), 65u);
    EXPECT_EQ(minutesIntoPeriod(at(kDay + 1, 1, 0), w), 180u);
    EXPECT_EQ(minutesIntoPeriod(at(kDay + 1, 8, 10), w), 10u);
}

TEST(ProbeMode, NamesAndGroups)
{
    EXPECT_STREQ(modeName(Mode::HeartSparse), "Heart 1 in 5");
    EXPECT_STREQ(modeCode(Mode::Off), "off");
    EXPECT_EQ(groups(Mode::Off), static_cast<uint32_t>(kGroupBattery));
    EXPECT_TRUE(groups(Mode::Full) & kGroupBeats);
    EXPECT_FALSE(groups(Mode::Motion) & kGroupHeart);
    EXPECT_EQ(rowMinutes(Mode::Off), 10u);
    EXPECT_EQ(rowMinutes(Mode::Full), 1u);
    EXPECT_EQ(modeFrom(77), Mode::Full);
    for (uint8_t i = 0; i < kModeCount; ++i) {
        EXPECT_LE(std::char_traits<char>::length(modeName(static_cast<Mode>(i))), 12u);
    }
}
