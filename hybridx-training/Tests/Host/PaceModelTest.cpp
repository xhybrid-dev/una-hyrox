#include "PaceModel.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdlib>

using namespace XTraining;
using namespace XTraining::PaceConfig;

namespace {

int diff(uint32_t a, uint32_t b) { return std::abs(static_cast<int>(a) - static_cast<int>(b)); }

// Daniels' training-pace table rows found in the review (REVIEW.md section 1),
// from secondary sources; s/km, and s per 400 m for repetition. Replace with
// rows read from the book (NOTES.md, open items) and keep the +/-3 s/km rule.
struct Row {
    float    fitness;
    uint16_t easyFast, easySlow;  // 0 = not in the source
    uint16_t threshold;
    uint16_t interval;
    uint16_t rep400;
};
constexpr Row kRows[] = {
    {51.0f, 4 * 60 + 52, 5 * 60 + 29, 4 * 60 + 11, 3 * 60 + 51, 86},
    {40.0f, 5 * 60 + 59, 6 * 60 + 44, 5 * 60 + 6, 0, 108},
    {45.0f, 0, 0, 4 * 60 + 38, 0, 96},
};

}  // namespace

TEST(PaceModel, TrainingPacesMatchThePublishedRows)
{
    for (const Row& r : kRows) {
        SCOPED_TRACE(r.fitness);
        if (r.easyFast != 0) {
            EXPECT_LE(diff(PaceModel::paceAt(r.fitness, kEasyFast), r.easyFast), 3);
            EXPECT_LE(diff(PaceModel::paceAt(r.fitness, kEasySlow), r.easySlow), 3);
        }
        EXPECT_LE(diff(PaceModel::paceAt(r.fitness, kThreshold), r.threshold), 3);
        const uint16_t interval = PaceModel::paceAt(r.fitness, kInterval);
        if (r.interval != 0) {
            EXPECT_LE(diff(interval, r.interval), 3);
        }
        const uint32_t rep400 = (static_cast<uint32_t>(interval - kRepetitionFasterSPerKm) * 4u + 5u) / 10u;
        EXPECT_LE(diff(rep400, r.rep400), 2);
    }
}

TEST(PaceModel, RaceTableAtFitness50)
{
    // Daniels' published race table for 50: 19:57, 41:21, 1:31:35, 3:10:49
    // (RACE_PREDICTIONS.md section 4.3). The equations land within 10 s.
    EXPECT_LE(diff(PaceModel::raceSeconds(50.0f, 5000.0f), 19 * 60 + 57), 10);
    EXPECT_LE(diff(PaceModel::raceSeconds(50.0f, 10000.0f), 41 * 60 + 21), 10);
    EXPECT_LE(diff(PaceModel::raceSeconds(50.0f, kHalfM), 3600 + 31 * 60 + 35), 10);
    EXPECT_LE(diff(PaceModel::raceSeconds(50.0f, kMarathonM), 3 * 3600 + 10 * 60 + 49), 10);
}

TEST(PaceModel, RaceTimeInvertsFitness)
{
    for (uint32_t t = kMin5kSec; t <= kMax5kSec; t += 7) {
        const float f = PaceModel::fitness(5000.0f, t);
        ASSERT_LE(diff(PaceModel::raceSeconds(f, 5000.0f), t), 1) << t;
    }
}

TEST(PaceModel, AgreesWithRiegelWithinOnePercent)
{
    // Riegel, b = 1.06 (RACE_PREDICTIONS.md section 5), the review's cross-check.
    for (uint32_t t5 = 18 * 60; t5 <= 36 * 60; t5 += 30) {
        const PaceResult r = PaceModel::compute({t5, 0});
        ASSERT_TRUE(r.ok);
        const double riegel10 = t5 * std::pow(2.0, 1.06);
        const double riegelHalf = t5 * std::pow(kHalfM / 5000.0, 1.06);
        EXPECT_LT(std::fabs(r.predicted10kSec - riegel10) / riegel10, 0.01) << t5;
        EXPECT_LT(std::fabs(r.predictedHalfSec - riegelHalf) / riegelHalf, 0.01) << t5;
    }
}

TEST(PaceModel, TheReviewsExampleRunner)
{
    // PLANS.md section 2.3, the 22:00 row.
    const PaceResult r = PaceModel::compute({22 * 60, 0});
    ASSERT_TRUE(r.ok);
    EXPECT_NEAR(r.fitness5k, 44.5f, 0.05f);
    EXPECT_EQ(r.paces.easyFast, 5 * 60 + 27);
    EXPECT_EQ(r.paces.easySlow, 6 * 60 + 8);
    EXPECT_EQ(r.paces.marathon, 4 * 60 + 59);
    EXPECT_EQ(r.paces.half, 4 * 60 + 48);
    EXPECT_EQ(r.paces.threshold, 4 * 60 + 40);
    EXPECT_EQ(r.paces.tenK, 4 * 60 + 34);
    EXPECT_EQ(r.paces.interval, 4 * 60 + 18);
    EXPECT_EQ(r.paces.repetition, 4 * 60 + 3);
    EXPECT_LE(diff(r.predicted10kSec, 45 * 60 + 37), 1);
    EXPECT_LE(diff(r.predictedHalfSec, 3600 + 41 * 60 + 7), 1);
    EXPECT_FALSE(r.enduranceGap);
    EXPECT_EQ(r.halfAs5kSec, 0u);
}

TEST(PaceModel, OrderAndMonotonicOverTheWholeRange)
{
    Paces prev{};
    for (uint32_t t = kMin5kSec; t <= kMax5kSec; ++t) {
        const PaceResult r = PaceModel::compute({t, 0});
        ASSERT_TRUE(r.ok) << t;
        const Paces& p = r.paces;
        ASSERT_GT(p.easySlow, p.easyFast) << t;
        ASSERT_GT(p.easyFast, p.marathon) << t;
        ASSERT_GE(p.marathon, p.half) << t;
        ASSERT_GT(p.interval, p.repetition) << t;
        ASSERT_GT(p.threshold, p.interval) << t;
        ASSERT_GT(p.tenK, p.interval) << t;
        ASSERT_GT(p.repetition, 120) << t;   // never absurdly fast
        ASSERT_LT(p.easySlow, 1200) << t;    // nor absurdly slow (20:00 /km)
        if (t > kMin5kSec) {
            ASSERT_GE(p.easyFast, prev.easyFast) << t;
            ASSERT_GE(p.threshold, prev.threshold) << t;
            ASSERT_GE(p.interval, prev.interval) << t;
        }
        prev = p;
    }
}

TEST(PaceModel, OutOfRangeFiveK)
{
    EXPECT_FALSE(PaceModel::compute({0, 0}).ok);
    EXPECT_FALSE(PaceModel::compute({kMin5kSec - 1, 0}).ok);
    EXPECT_FALSE(PaceModel::compute({kMax5kSec + 1, 0}).ok);
}

TEST(PaceModel, TheHalfSetsHalfPaceAndEasyAndShowsTheGap)
{
    // PLANS.md 2.4: 22:00 and 1:45:00, a half that matches a 22:50 5K.
    const PaceResult r = PaceModel::compute({22 * 60, 3600 + 45 * 60});
    ASSERT_TRUE(r.ok);
    EXPECT_NEAR(r.fitnessHalf, 42.6f, 0.05f);
    EXPECT_LE(diff(r.halfAs5kSec, 22 * 60 + 50), 2);
    EXPECT_EQ(r.paces.half, 4 * 60 + 59);       // from the half itself
    EXPECT_EQ(r.paces.easyFast, 5 * 60 + 39);   // from the half's fitness
    EXPECT_EQ(r.paces.easySlow, 6 * 60 + 21);
    EXPECT_EQ(r.paces.threshold, 4 * 60 + 40);  // still the 5K's
    EXPECT_EQ(r.paces.interval, 4 * 60 + 18);
    // 22:50 is 3.8% slower than 22:00: under the 4% line.
    EXPECT_FALSE(r.enduranceGap);

    const PaceResult gap = PaceModel::compute({22 * 60, 3600 + 50 * 60});
    EXPECT_TRUE(gap.enduranceGap);
}

TEST(PaceModel, AFastHalfDoesNotSpeedUpEasyRunning)
{
    const PaceResult only5k = PaceModel::compute({25 * 60, 0});
    const PaceResult fastHalf = PaceModel::compute({25 * 60, 3600 + 40 * 60});
    EXPECT_EQ(fastHalf.paces.easyFast, only5k.paces.easyFast);
    EXPECT_FALSE(fastHalf.enduranceGap);
    EXPECT_LT(fastHalf.paces.half, only5k.paces.half);
}

TEST(PaceModel, Bands)
{
    const SpeedBand b = PaceModel::band(280, 5);  // 4:40 /km +/- 5 s
    EXPECT_EQ(b.lowMmps, PaceModel::mmps(285));
    EXPECT_EQ(b.highMmps, PaceModel::mmps(275));
    EXPECT_EQ(PaceModel::mmps(250), 4000u);
    EXPECT_LT(b.lowMmps, b.highMmps);
    EXPECT_EQ(PaceModel::mmps(0), 0u);
}
