/**
 * Running form: average cadence and step length of a run and of each lap, and
 * the text the screens show.
 */

#include <gtest/gtest.h>

#include <cstring>

#include "FormStats.hpp"
#include "FormText.hpp"

using namespace RunVo2;

namespace
{

FormSecond second(uint16_t cadenceX10, uint16_t stepMm)
{
    FormSecond s;
    s.cadenceValid = cadenceX10 != 0;
    s.cadenceX10   = cadenceX10;
    s.stepValid    = stepMm != 0;
    s.stepMm       = stepMm;
    return s;
}

void feed(FormStats& f, const FormSecond& s, unsigned seconds)
{
    for (unsigned i = 0; i < seconds; ++i) {
        f.addSecond(s);
    }
}

}  // namespace

TEST(FormStats, NothingFedMeansNoData)
{
    FormStats f;
    EXPECT_EQ(f.cadenceAvgX10(), 0);
    EXPECT_EQ(f.stepAvgMm(), 0);
    EXPECT_EQ(f.lapCadenceAvgX10(), 0);
    EXPECT_EQ(f.lapStepAvgMm(), 0);
}

TEST(FormStats, ASteadyRunAveragesToItself)
{
    FormStats f;
    feed(f, second(1720, 1080), 600);
    EXPECT_EQ(f.cadenceAvgX10(), 1720);
    EXPECT_EQ(f.stepAvgMm(), 1080);
    EXPECT_EQ(f.cadenceSeconds(), 600u);
}

TEST(FormStats, CadenceIsTheMeanOverValidSeconds)
{
    FormStats f;
    feed(f, second(1600, 0), 60);
    feed(f, second(1800, 0), 60);
    EXPECT_EQ(f.cadenceAvgX10(), 1700);
    EXPECT_EQ(f.stepAvgMm(), 0);   // no step length was ever valid
}

TEST(FormStats, SecondsWithoutCadenceAreLeftOut)
{
    FormStats f;
    feed(f, second(1700, 1000), 100);
    FormSecond none;   // a stalled sensor
    feed(f, none, 500);
    EXPECT_EQ(f.cadenceAvgX10(), 1700);
    EXPECT_EQ(f.cadenceSeconds(), 100u);
}

TEST(FormStats, AStepLengthWithoutACadenceIsIgnored)
{
    FormStats f;
    FormSecond s;
    s.cadenceValid = false;
    s.stepValid    = true;
    s.stepMm       = 1200;
    feed(f, s, 30);
    EXPECT_EQ(f.cadenceAvgX10(), 0);
    EXPECT_EQ(f.stepAvgMm(), 0);
}

TEST(FormStats, StepLengthIsDistanceOverSteps)
{
    // 60 s at 160 spm with 0.90 m steps, 60 s at 180 spm with 1.20 m steps.
    // Distance = 160 x 0.9 + 180 x 1.2 = 144 + 216 = 360 m; steps = 160 + 180 = 340.
    // 360 / 340 = 1.0588 m. A plain mean of the two step lengths would say 1.05 m.
    FormStats f;
    feed(f, second(1600, 900), 60);
    feed(f, second(1800, 1200), 60);
    EXPECT_EQ(f.stepAvgMm(), 1059);
    EXPECT_EQ(f.cadenceAvgX10(), 1700);
}

TEST(FormStats, ALapKeepsItsOwnTotalsAndTheRunKeepsAll)
{
    FormStats f;
    feed(f, second(1600, 1000), 60);   // lap 1
    EXPECT_EQ(f.lapCadenceAvgX10(), 1600);
    f.resetLap();
    EXPECT_EQ(f.lapCadenceAvgX10(), 0);
    EXPECT_EQ(f.lapStepAvgMm(), 0);
    feed(f, second(1800, 1200), 60);   // lap 2
    EXPECT_EQ(f.lapCadenceAvgX10(), 1800);
    EXPECT_EQ(f.lapStepAvgMm(), 1200);
    EXPECT_EQ(f.cadenceAvgX10(), 1700);   // the run: both laps
    EXPECT_EQ(f.cadenceSeconds(), 120u);
    EXPECT_EQ(f.lapCadenceSeconds(), 60u);
}

TEST(FormStats, AResetStartsANewRun)
{
    FormStats f;
    feed(f, second(1700, 1000), 60);
    f.reset();
    EXPECT_EQ(f.cadenceAvgX10(), 0);
    EXPECT_EQ(f.lapCadenceAvgX10(), 0);
    EXPECT_EQ(f.stepAvgMm(), 0);
}

TEST(FormStats, AveragesRoundToTheNearest)
{
    FormStats f;
    f.addSecond(second(1700, 1000));
    f.addSecond(second(1701, 1001));
    f.addSecond(second(1701, 1001));
    EXPECT_EQ(f.cadenceAvgX10(), 1701);   // 5102 / 3 = 1700.67
    EXPECT_EQ(f.stepAvgMm(), 1001);
}

TEST(FormStats, ALongRunDoesNotOverflow)
{
    // 24 hours at the top of the range: the totals fit (2.6e8 in a uint32; 64-bit weights).
    FormStats f;
    feed(f, second(3000, 2500), 86400);
    EXPECT_EQ(f.cadenceAvgX10(), 3000);
    EXPECT_EQ(f.stepAvgMm(), 2500);
}

TEST(FormText, CadenceIsWholeStepsPerMinute)
{
    char b[8];
    FormText::cadence(b, sizeof(b), 1720);
    EXPECT_STREQ(b, "172");
    FormText::cadence(b, sizeof(b), 1724);
    EXPECT_STREQ(b, "172");
    FormText::cadence(b, sizeof(b), 1725);
    EXPECT_STREQ(b, "173");   // rounded
    FormText::cadence(b, sizeof(b), 0);
    EXPECT_STREQ(b, "---");
    FormText::cadence(b, sizeof(b), 3000);
    EXPECT_STREQ(b, "300");
}

TEST(FormText, StepLengthIsMetresToTheCentimetre)
{
    char b[8];
    FormText::stepLength(b, sizeof(b), 1080);
    EXPECT_STREQ(b, "1.08");
    FormText::stepLength(b, sizeof(b), 1084);
    EXPECT_STREQ(b, "1.08");
    FormText::stepLength(b, sizeof(b), 1085);
    EXPECT_STREQ(b, "1.09");
    FormText::stepLength(b, sizeof(b), 150);
    EXPECT_STREQ(b, "0.15");   // the SDK's lower gate
    FormText::stepLength(b, sizeof(b), 2500);
    EXPECT_STREQ(b, "2.50");   // and its upper
    FormText::stepLength(b, sizeof(b), 0);
    EXPECT_STREQ(b, "---");
}

TEST(FormText, EveryInputIsBoundedByTheBuffer)
{
    // Whatever the value, the text stays inside the buffer and the return is its length.
    char b[8];
    for (const uint32_t v : {0u, 1u, 999u, 1700u, 3000u, 65535u}) {
        std::memset(b, 'x', sizeof(b));
        const size_t n = FormText::cadence(b, sizeof(b), static_cast<uint16_t>(v));
        EXPECT_EQ(n, std::strlen(b)) << v;
        EXPECT_LT(n, sizeof(b)) << v;
        std::memset(b, 'x', sizeof(b));
        const size_t m = FormText::stepLength(b, sizeof(b), static_cast<uint16_t>(v));
        EXPECT_EQ(m, std::strlen(b)) << v;
        EXPECT_LT(m, sizeof(b)) << v;
    }
    // The widest real values are four characters: "300" and "2.50".
    FormText::cadence(b, sizeof(b), 3000);
    EXPECT_LE(std::strlen(b), 4u);
    FormText::stepLength(b, sizeof(b), 2500);
    EXPECT_LE(std::strlen(b), 4u);

    // A buffer too small for the text is cut, never overrun.
    char tiny[3];
    const size_t n = FormText::stepLength(tiny, sizeof(tiny), 1080);
    EXPECT_EQ(n, std::strlen(tiny));
    EXPECT_LT(n, sizeof(tiny));
    EXPECT_EQ(FormText::cadence(nullptr, 4, 1700), 0u);
    char none[1];
    EXPECT_EQ(FormText::cadence(none, 0, 1700), 0u);
}
