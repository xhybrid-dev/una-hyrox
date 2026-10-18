// The Daily Trail's seed: one per date, the same for everyone.

#include <gtest/gtest.h>

#include <set>

#include "DailyTrail.hpp"

using namespace TrailQuest;

TEST(DailyTrail, TheSameDateGivesTheSameSeed)
{
    EXPECT_EQ(dailySeed(2026, 10, 17), dailySeed(2026, 10, 17));
    EXPECT_NE(dailySeed(2026, 10, 17), 0u);
}

TEST(DailyTrail, EveryDayOfTwoYearsHasItsOwnSeed)
{
    static const uint8_t kDays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    std::set<uint32_t> seeds;
    int days = 0;
    for (uint16_t y = 2026; y <= 2027; ++y) {
        for (uint8_t m = 1; m <= 12; ++m) {
            for (uint8_t d = 1; d <= kDays[m - 1]; ++d) {
                seeds.insert(dailySeed(y, m, d));
                ++days;
            }
        }
    }
    EXPECT_EQ(seeds.size(), static_cast<size_t>(days));
}

TEST(DailyTrail, NeighbouringDatesAreNotNeighbouringSeeds)
{
    const uint32_t a = dailySeed(2026, 10, 17);
    const uint32_t b = dailySeed(2026, 10, 18);
    EXPECT_GT(a > b ? a - b : b - a, 1000u);
}
