// HybridX Streak: fitness, fatigue and form from the apps' training loads (LoadHistory.hpp).

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "LoadHistory.hpp"

using namespace Streak;

namespace {

constexpr int32_t kDay0 = 20400;   // a day number in 2025; any will do

/// The same model in floating point, with no rounding: what the integers must track.
struct Reference {
    double fitness = 0;
    double fatigue = 0;
    void day(double load)
    {
        fitness += (load - fitness) / 42.0;
        fatigue += (load - fatigue) / 7.0;
    }
};

} // namespace

TEST(LoadHistory, NothingKeptMeansNothingShown)
{
    LoadState s;
    LoadHistory h(s);
    const LoadFigures f = h.figures(kDay0);
    EXPECT_EQ(f.fitnessX100, 0u);
    EXPECT_EQ(f.formX100, 0);
    EXPECT_EQ(f.word, FormWord::Learning);
}

TEST(LoadHistory, TracksTheFloatingPointModelOverAYear)
{
    // A training pattern: 50 on three days a week, 90 on Saturdays, rest on Sundays.
    auto loadOf = [](int32_t d) -> uint16_t {
        return (d % 7 == 5) ? 90 : ((d % 7 == 0 || d % 7 == 2 || d % 7 == 3) ? 50 : 0);
    };
    // The exact model, seeded as LoadHistory is: both averages at the warm-up's mean.
    double mean = 0;
    for (int32_t d = 0; d < kWarmUpDays; ++d) {
        mean += loadOf(d);
    }
    mean /= kWarmUpDays;
    Reference ref;
    ref.fitness = mean;
    ref.fatigue = mean;

    LoadState s;
    LoadHistory h(s);
    for (int32_t d = 0; d < 365; ++d) {
        const int32_t day = kDay0 + d;
        h.advanceTo(day);
        h.add(day, loadOf(d));
        ref.day(loadOf(d));
        if (d < kWarmUpDays - 1) {
            continue;   // learning: not seeded yet
        }
        const LoadFigures f = h.figures(day);
        // Within a tenth of a point of the exact model, every day.
        EXPECT_NEAR(f.fitnessX100 / 100.0, ref.fitness, 0.1) << d;
        EXPECT_NEAR(f.fatigueX100 / 100.0, ref.fatigue, 0.1) << d;
        EXPECT_NEAR(f.formX100 / 100.0, ref.fitness - ref.fatigue, 0.2) << d;
    }
}

TEST(LoadHistory, ALateSessionLandsOnItsOwnDay)
{
    // Streak opened today finds a session from five days ago: the figures are as if it
    // had been found that day.
    LoadState late, onTime;
    LoadHistory a(late), b(onTime);
    const int32_t today = kDay0 + 30;
    for (int32_t d = kDay0; d <= today; ++d) {
        b.advanceTo(d);
        if (d == today - 5) {
            b.add(d, 60);
        }
    }
    a.advanceTo(kDay0);
    a.advanceTo(today);
    a.add(today - 5, 60);
    const LoadFigures fa = a.figures(today);
    const LoadFigures fb = b.figures(today);
    EXPECT_EQ(fa.fitnessX100, fb.fitnessX100);
    EXPECT_EQ(fa.fatigueX100, fb.fatigueX100);
    EXPECT_EQ(a.dayLoad(today - 5), 60);
}

TEST(LoadHistory, ASessionOlderThanTheWindowGoesOnItsOldestDay)
{
    LoadState s;
    LoadHistory h(s);
    h.advanceTo(kDay0 + 40);
    h.add(kDay0 + 40 - 20, 30);    // 20 days ago: before the 14 open days
    EXPECT_EQ(h.dayLoad(kDay0 + 40 - 13), 30);
    EXPECT_EQ(h.sum(kDay0, kDay0 + 40), 30u);
}

TEST(LoadHistory, AFutureDayGoesOnToday)
{
    LoadState s;
    LoadHistory h(s);
    h.advanceTo(kDay0);
    h.add(kDay0 + 3, 25);          // a clock problem: never lost, never ahead
    EXPECT_EQ(h.dayLoad(kDay0), 25);
}

TEST(LoadHistory, TheClockGoingBackDoesNotMoveTheWindowBack)
{
    LoadState s;
    LoadHistory h(s);
    h.advanceTo(kDay0 + 20);
    h.add(kDay0 + 20, 40);
    h.advanceTo(kDay0 + 10);
    EXPECT_EQ(h.dayLoad(kDay0 + 20), 40);
    EXPECT_EQ(s.openStart, kDay0 + 20 - (kOpenDays - 1));
}

TEST(LoadHistory, ALongGapDecaysToNothing)
{
    LoadState s;
    LoadHistory h(s);
    for (int32_t d = 0; d < 60; ++d) {
        h.advanceTo(kDay0 + d);
        h.add(kDay0 + d, 100);
    }
    EXPECT_GT(h.figures(kDay0 + 59).fitnessX100, 6000u);
    // A year away: nearly nothing left, and the fast path (beyond kMaxFold) is exactly nothing.
    h.advanceTo(kDay0 + 59 + 365);
    EXPECT_LT(h.figures(kDay0 + 59 + 365).fitnessX100, 100u);
    h.advanceTo(kDay0 + 59 + 365 + LoadHistory::kMaxFold + 5);
    const LoadFigures f = h.figures(kDay0 + 59 + 365 + LoadHistory::kMaxFold + 5);
    EXPECT_EQ(f.fitnessX100, 0u);
    EXPECT_EQ(f.fatigueX100, 0u);
    EXPECT_EQ(f.word, FormWord::NoLoad);
}

TEST(LoadHistory, TheWordWaitsForTwoWeeksOfHistory)
{
    LoadState s;
    LoadHistory h(s);
    for (int32_t d = 0; d < kWarmUpDays - 1; ++d) {
        h.advanceTo(kDay0 + d);
        h.add(kDay0 + d, 50);
        EXPECT_EQ(h.figures(kDay0 + d).word, FormWord::Learning) << d;
        EXPECT_EQ(h.figures(kDay0 + d).historyDays, d + 1);
    }
    h.advanceTo(kDay0 + kWarmUpDays - 1);
    EXPECT_NE(h.figures(kDay0 + kWarmUpDays - 1).word, FormWord::Learning);
}

TEST(LoadHistory, TheWordsFollowFormAsAShareOfFitness)
{
    EXPECT_EQ(LoadMath::wordFor(0, 0, 30), FormWord::NoLoad);
    EXPECT_EQ(LoadMath::wordFor(5000, 4000, 30), FormWord::Fresh);       // +20 %
    EXPECT_EQ(LoadMath::wordFor(5000, 5400, 30), FormWord::Steady);      // -8 %
    EXPECT_EQ(LoadMath::wordFor(5000, 5500, 30), FormWord::Steady);      // -10 %: the edge is Steady
    EXPECT_EQ(LoadMath::wordFor(5000, 6000, 30), FormWord::Building);    // -20 %
    EXPECT_EQ(LoadMath::wordFor(5000, 6500, 30), FormWord::Building);    // -30 %
    EXPECT_EQ(LoadMath::wordFor(5000, 7000, 30), FormWord::Overreaching);// -40 %
    EXPECT_EQ(LoadMath::wordFor(0, 500, 30), FormWord::Overreaching);    // fatigue with no fitness
    EXPECT_EQ(LoadMath::wordFor(5000, 4000, 13), FormWord::Learning);
}

TEST(LoadHistory, ABlockOfHardTrainingThenATaperReadsAsItShould)
{
    // Four weeks steady, two weeks hard, one week of taper: Steady, then Building or
    // Overreaching, then Fresh.
    LoadState s;
    LoadHistory h(s);
    int32_t d = 0;
    auto run = [&](int days, uint16_t load) {
        for (int i = 0; i < days; ++i, ++d) {
            h.advanceTo(kDay0 + d);
            h.add(kDay0 + d, load);
        }
    };
    run(28, 60);
    EXPECT_EQ(h.figures(kDay0 + d - 1).word, FormWord::Steady);
    run(14, 150);
    const FormWord hard = h.figures(kDay0 + d - 1).word;
    EXPECT_TRUE(hard == FormWord::Building || hard == FormWord::Overreaching);
    EXPECT_LT(h.figures(kDay0 + d - 1).formX100, 0);
    run(7, 15);
    EXPECT_EQ(h.figures(kDay0 + d - 1).word, FormWord::Fresh);
    EXPECT_GT(h.figures(kDay0 + d - 1).formX100, 0);
}

TEST(LoadHistory, ADaysLoadSaturates)
{
    LoadState s;
    LoadHistory h(s);
    h.advanceTo(kDay0);
    h.add(kDay0, 60000);
    h.add(kDay0, 60000);
    EXPECT_EQ(h.dayLoad(kDay0), 0xFFFF);
}
