/**
 * @file RaceTemplateTest.cpp
 * @brief Segment counts, ordering, round numbering and labels (brief 7.2, 12.1).
 */

#include <gtest/gtest.h>

#include "RaceData.hpp"
#include "RaceModel.hpp"

#include <cstring>

using Race::Format;
using Race::RaceModel;
using Race::SegmentDesc;
using Race::SegmentType;

namespace
{

std::string labelOf(const SegmentDesc &desc)
{
    char buf[Race::kMaxLabelLen] = {};
    RaceModel::label(desc, buf, sizeof(buf));
    return std::string(buf);
}

std::string nameOf(const SegmentDesc &desc)
{
    char buf[Race::kMaxNameLen] = {};
    RaceModel::name(desc, buf, sizeof(buf));
    return std::string(buf);
}

}  // namespace

// -- Counts (brief 7.5 invariant 7) -------------------------------------------

TEST(RaceTemplateTest, SegmentCountsMatchTheBrief)
{
    EXPECT_EQ(RaceModel::plannedCount(Format::Full, false), 16u);
    EXPECT_EQ(RaceModel::plannedCount(Format::Full, true), 31u);
    EXPECT_EQ(RaceModel::plannedCount(Format::HalfA, false), 8u);
    EXPECT_EQ(RaceModel::plannedCount(Format::HalfA, true), 15u);
    EXPECT_EQ(RaceModel::plannedCount(Format::HalfB, false), 8u);
    EXPECT_EQ(RaceModel::plannedCount(Format::HalfB, true), 15u);
}

TEST(RaceTemplateTest, BuildTemplateWritesThatManySegments)
{
    SegmentDesc plan[Race::kMaxSegments] = {};

    EXPECT_EQ(RaceModel::buildTemplate(Format::Full, false, plan, Race::kMaxSegments), 16u);
    EXPECT_EQ(RaceModel::buildTemplate(Format::Full, true, plan, Race::kMaxSegments), 31u);
    EXPECT_EQ(RaceModel::buildTemplate(Format::HalfA, true, plan, Race::kMaxSegments), 15u);
    EXPECT_EQ(RaceModel::buildTemplate(Format::HalfB, true, plan, Race::kMaxSegments), 15u);
}

// -- Ordering ------------------------------------------------------------------

TEST(RaceTemplateTest, RoxzoneOffAlternatesRunAndStation)
{
    SegmentDesc plan[Race::kMaxSegments] = {};
    const uint8_t n = RaceModel::buildTemplate(Format::Full, false, plan, Race::kMaxSegments);
    ASSERT_EQ(n, 16u);

    for (uint8_t round = 1u; round <= 8u; ++round) {
        const uint8_t i = static_cast<uint8_t>((round - 1u) * 2u);
        EXPECT_EQ(plan[i].type, SegmentType::Run) << "at round " << int(round);
        EXPECT_EQ(plan[i].round, round);
        EXPECT_EQ(plan[i].stationId, 0u) << "a run is not a station";

        EXPECT_EQ(plan[i + 1u].type, SegmentType::Station);
        EXPECT_EQ(plan[i + 1u].round, round);
        EXPECT_EQ(plan[i + 1u].stationId, round);
    }
}

TEST(RaceTemplateTest, RoxzoneOnWrapsEachStation)
{
    SegmentDesc plan[Race::kMaxSegments] = {};
    const uint8_t n = RaceModel::buildTemplate(Format::Full, true, plan, Race::kMaxSegments);
    ASSERT_EQ(n, 31u);

    // Rounds 1 to 7 are the full four-segment cycle.
    for (uint8_t round = 1u; round <= 7u; ++round) {
        const uint8_t i = static_cast<uint8_t>((round - 1u) * 4u);
        EXPECT_EQ(plan[i].type, SegmentType::Run) << "at round " << int(round);
        EXPECT_EQ(plan[i + 1u].type, SegmentType::RoxIn);
        EXPECT_EQ(plan[i + 2u].type, SegmentType::Station);
        EXPECT_EQ(plan[i + 3u].type, SegmentType::RoxOut);
    }

    // Round 8 has no ROX_OUT: the race ends when the last station ends.
    EXPECT_EQ(plan[28].type, SegmentType::Run);
    EXPECT_EQ(plan[29].type, SegmentType::RoxIn);
    EXPECT_EQ(plan[30].type, SegmentType::Station);
    EXPECT_EQ(plan[30].stationId, 8u) << "the race must end on Wall Balls";
}

TEST(RaceTemplateTest, NoRoxOutFollowsTheFinalStationInAnyFormat)
{
    for (const Format format : { Format::Full, Format::HalfA, Format::HalfB }) {
        SegmentDesc plan[Race::kMaxSegments] = {};
        const uint8_t n = RaceModel::buildTemplate(format, true, plan, Race::kMaxSegments);
        ASSERT_GT(n, 0u);
        EXPECT_EQ(plan[n - 1u].type, SegmentType::Station)
                << "format " << int(static_cast<uint8_t>(format));
    }
}

// -- Round numbering in halves (brief 7.2) ------------------------------------

TEST(RaceTemplateTest, HalfARunsRoundsOneToFour)
{
    SegmentDesc plan[Race::kMaxSegments] = {};
    const uint8_t n = RaceModel::buildTemplate(Format::HalfA, false, plan, Race::kMaxSegments);
    ASSERT_EQ(n, 8u);

    EXPECT_EQ(plan[0].round, 1u);
    EXPECT_EQ(plan[7].round, 4u);
    EXPECT_EQ(plan[7].stationId, 4u) << "ends on Burpee Broad Jumps";
}

TEST(RaceTemplateTest, HalfBKeepsRealRoundNumbers)
{
    SegmentDesc plan[Race::kMaxSegments] = {};
    const uint8_t n = RaceModel::buildTemplate(Format::HalfB, false, plan, Race::kMaxSegments);
    ASSERT_EQ(n, 8u);

    // The athlete is doing rounds 5 to 8, and the watch must say so.
    EXPECT_EQ(plan[0].round, 5u);
    EXPECT_EQ(plan[0].type, SegmentType::Run);
    EXPECT_EQ(labelOf(plan[0]), "RUN 5/8 \xC2\xB7 1 km");

    EXPECT_EQ(plan[1].stationId, 5u) << "first station of the second half is Row";
    EXPECT_EQ(plan[7].round, 8u);
    EXPECT_EQ(plan[7].stationId, 8u);
}

// -- Labels (brief 7.2) --------------------------------------------------------

TEST(RaceTemplateTest, LabelsReadAsSpecified)
{
    EXPECT_EQ(labelOf({ SegmentType::Run, 3u, 0u }), "RUN 3/8 \xC2\xB7 1 km");
    EXPECT_EQ(labelOf({ SegmentType::RoxIn, 3u, 0u }), "ROXZONE IN");
    EXPECT_EQ(labelOf({ SegmentType::Station, 3u, 3u }), "SLED PULL \xC2\xB7 50 m");
    EXPECT_EQ(labelOf({ SegmentType::RoxOut, 3u, 0u }), "ROXZONE OUT");
}

TEST(RaceTemplateTest, EveryStationLabelsWithItsConfirmedWork)
{
    // Jon confirmed this table on 21 September 2026; it is the reason the app
    // exists, so it gets an explicit test rather than a loop over kStations.
    EXPECT_EQ(labelOf({ SegmentType::Station, 1u, 1u }), "SKIERG \xC2\xB7 1000 m");
    EXPECT_EQ(labelOf({ SegmentType::Station, 2u, 2u }), "SLED PUSH \xC2\xB7 50 m");
    EXPECT_EQ(labelOf({ SegmentType::Station, 3u, 3u }), "SLED PULL \xC2\xB7 50 m");
    EXPECT_EQ(labelOf({ SegmentType::Station, 4u, 4u }), "BURPEE BROAD JUMPS \xC2\xB7 80 m");
    EXPECT_EQ(labelOf({ SegmentType::Station, 5u, 5u }), "ROW \xC2\xB7 1000 m");
    EXPECT_EQ(labelOf({ SegmentType::Station, 6u, 6u }), "FARMERS CARRY \xC2\xB7 200 m");
    EXPECT_EQ(labelOf({ SegmentType::Station, 7u, 7u }), "SANDBAG LUNGES \xC2\xB7 100 m");
    EXPECT_EQ(labelOf({ SegmentType::Station, 8u, 8u }), "WALL BALLS \xC2\xB7 100 reps");
}

TEST(RaceTemplateTest, NamesDropTheWorkAndTheSeparator)
{
    // The race face and the split toast show the name alone; label() is what
    // the FIT lap and the summary heading keep.
    EXPECT_EQ(nameOf({ SegmentType::Run, 3u, 0u }), "RUN 3/8");
    EXPECT_EQ(nameOf({ SegmentType::RoxIn, 3u, 0u }), "ROXZONE IN");
    EXPECT_EQ(nameOf({ SegmentType::Station, 3u, 3u }), "SLED PULL");
    EXPECT_EQ(nameOf({ SegmentType::RoxOut, 3u, 0u }), "ROXZONE OUT");
    EXPECT_EQ(nameOf({ SegmentType::Station, 9u, 9u }), "STATION")
            << "a corrupt index must not read past kStations";
}

TEST(RaceTemplateTest, WorkIsTheHalfOfTheLabelNameLeavesOut)
{
    EXPECT_STREQ(RaceModel::work({ SegmentType::Run, 3u, 0u }), "1 km");
    EXPECT_STREQ(RaceModel::work({ SegmentType::Station, 8u, 8u }), "100 reps");
    EXPECT_STREQ(RaceModel::work({ SegmentType::RoxIn, 3u, 0u }), "")
            << "a Roxzone segment has no work";
    EXPECT_STREQ(RaceModel::work({ SegmentType::Station, 9u, 9u }), "")
            << "and neither does a segment that does not exist";
}

TEST(RaceTemplateTest, NameToleratesAZeroSizedBuffer)
{
    char buf[1] = { 'x' };
    RaceModel::name({ SegmentType::Run, 1u, 0u }, buf, 0u);
    EXPECT_EQ(buf[0], 'x') << "nothing should be written";

    RaceModel::name({ SegmentType::Run, 1u, 0u }, nullptr, Race::kMaxNameLen);
}

TEST(RaceTemplateTest, LongestStationNameFitsTheDeclaredBuffer)
{
    // kMaxNameLen sizes the race face's accent line, which shows the name
    // without its work. Overrunning it would truncate a station mid-word.
    for (uint8_t id = 0u; id < Race::kStationCount; ++id) {
        EXPECT_LT(std::strlen(Race::kStations[id].name), Race::kMaxNameLen)
                << "station " << int(id + 1u) << " name fills the buffer";
    }
}

TEST(RaceTemplateTest, EveryStationHasABriefNameThatFitsTheSplitRow)
{
    // The summary's split rows are the narrowest text on the watch. If a brief
    // name outgrows the budget it is the bottom row that silently clips.
    for (uint8_t id = 0u; id < Race::kStationCount; ++id) {
        ASSERT_NE(Race::kStations[id].brief, nullptr) << "station " << int(id + 1u);
        EXPECT_LT(std::strlen(Race::kStations[id].brief), Race::kMaxBriefLen)
                << "station " << int(id + 1u) << " brief name is too wide";
    }
}

TEST(RaceTemplateTest, LongestLabelFitsTheDeclaredBuffer)
{
    // kMaxLabelLen sizes buffers all over the GUI. If a label ever outgrows it
    // the text would be silently truncated on the watch, so pin it here.
    for (uint8_t id = 1u; id <= Race::kStationCount; ++id) {
        char buf[Race::kMaxLabelLen] = {};
        RaceModel::label({ SegmentType::Station, id, id }, buf, sizeof(buf));
        EXPECT_LT(std::strlen(buf), Race::kMaxLabelLen - 1u)
                << "station " << int(id) << " label fills the buffer";
    }
}

// -- Distance (brief 10.1, Jon's decision of 22 September 2026) ----------------

TEST(RaceTemplateTest, EverySegmentCreditsTheDistanceTheFormatStates)
{
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Run, 3u, 0u }), 1000u);

    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 1u, 1u }), 1000u) << "SkiErg";
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 2u, 2u }), 50u) << "Sled push";
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 3u, 3u }), 50u) << "Sled pull";
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 4u, 4u }), 80u) << "Burpees";
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 5u, 5u }), 1000u) << "Row";
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 6u, 6u }), 200u) << "Carry";
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 7u, 7u }), 100u) << "Lunges";

    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 8u, 8u }), 0u)
            << "Wall Balls are reps, not metres";
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::RoxIn, 3u, 0u }), 0u);
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::RoxOut, 3u, 0u }), 0u);
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 9u, 9u }), 0u)
            << "a corrupt index must not read past kStations";
}

namespace
{
uint32_t plannedDistanceM(Format format, bool roxzone)
{
    SegmentDesc plan[Race::kMaxSegments] = {};
    const uint8_t n = RaceModel::buildTemplate(format, roxzone, plan, Race::kMaxSegments);
    uint32_t total = 0u;
    for (uint8_t i = 0u; i < n; ++i) {
        total += RaceModel::distanceM(plan[i]);
    }
    return total;
}
}  // namespace

TEST(RaceTemplateTest, ARaceTotalsTheDistanceItIsSupposedTo)
{
    // This number is what Garmin Connect and Strava will show, so it is worth
    // pinning: eight kilometres of running plus every station's stated metres.
    EXPECT_EQ(plannedDistanceM(Format::Full, false), 10480u);
    EXPECT_EQ(plannedDistanceM(Format::HalfA, false), 5180u);
    EXPECT_EQ(plannedDistanceM(Format::HalfB, false), 5300u);

    // Roxzone splitting adds segments but no distance.
    EXPECT_EQ(plannedDistanceM(Format::Full, true), 10480u);
}

// -- Adjustable run distance (Jon's request, 23 September 2026) ---------------

TEST(RaceTemplateTest, RunWorkReadsAsTheDistanceAsked)
{
    EXPECT_STREQ(Race::runWork(1000u), "1 km") << "the race distance keeps its own wording";
    EXPECT_STREQ(Race::runWork(800u), "800 m");
    EXPECT_STREQ(Race::runWork(500u), "500 m");
    EXPECT_STREQ(Race::runWork(100u), "100 m");
}

TEST(RaceTemplateTest, RunDistanceIsClampedAndSnappedToAStep)
{
    EXPECT_EQ(Race::clampRunDistanceM(800u), 800u);
    EXPECT_EQ(Race::clampRunDistanceM(0u), Race::kRunDistanceMinM);
    EXPECT_EQ(Race::clampRunDistanceM(60000u), Race::kRunDistanceMaxM);
    EXPECT_EQ(Race::clampRunDistanceM(849u), 800u) << "rounded down to a 100 m step";

    // Whatever comes back must index the work table, which is what makes
    // runWork() safe on a corrupt setting.
    for (uint32_t m = 0u; m <= 1200u; ++m) {
        const uint16_t clamped = Race::clampRunDistanceM(static_cast<uint16_t>(m));
        EXPECT_GE(clamped, Race::kRunDistanceMinM) << "at " << m;
        EXPECT_LE(clamped, Race::kRunDistanceMaxM) << "at " << m;
        EXPECT_EQ(clamped % Race::kRunDistanceStepM, 0u) << "at " << m;
        EXPECT_STRNE(Race::runWork(static_cast<uint16_t>(m)), "") << "at " << m;
    }
}

TEST(RaceTemplateTest, AShortenedRunChangesOnlyTheRuns)
{
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Run, 3u, 0u }, 800u), 800u);
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 1u, 1u }, 800u), 1000u)
            << "the SkiErg is the SkiErg whatever the runs are";
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::Station, 8u, 8u }, 800u), 0u);
    EXPECT_EQ(RaceModel::distanceM({ SegmentType::RoxIn, 3u, 0u }, 800u), 0u);
}

TEST(RaceTemplateTest, ASimTotalsTheRightDistance)
{
    auto total = [](Format format, bool roxzone, uint16_t runM) {
        SegmentDesc plan[Race::kMaxSegments] = {};
        const uint8_t n = RaceModel::buildTemplate(format, roxzone, plan, Race::kMaxSegments);
        uint32_t sum = 0u;
        for (uint8_t i = 0u; i < n; ++i) {
            sum += RaceModel::distanceM(plan[i], runM);
        }
        return sum;
    };

    // 8 runs plus 2480 m of stations.
    EXPECT_EQ(total(Format::Full, false, 1000u), 10480u);
    EXPECT_EQ(total(Format::Full, false, 800u), 8880u);
    EXPECT_EQ(total(Format::Full, false, 500u), 6480u);

    // Half A is 4 runs plus SkiErg, sled push, sled pull and burpees.
    EXPECT_EQ(total(Format::HalfA, false, 500u), 2000u + 1180u);
}

TEST(RaceTemplateTest, LabelsFollowTheRunDistance)
{
    char buf[Race::kMaxLabelLen] = {};
    RaceModel::label({ SegmentType::Run, 3u, 0u }, buf, sizeof(buf), 800u);
    EXPECT_EQ(std::string(buf), "RUN 3/8 \xC2\xB7 800 m");

    RaceModel::label({ SegmentType::Station, 3u, 3u }, buf, sizeof(buf), 800u);
    EXPECT_EQ(std::string(buf), "SLED PULL \xC2\xB7 50 m") << "a station is untouched";

    EXPECT_STREQ(RaceModel::work({ SegmentType::Run, 1u, 0u }, 500u), "500 m");
}

// -- Defensive behaviour (no MMU, brief 14.14) --------------------------------

TEST(RaceTemplateTest, BuildTemplateRefusesTooSmallABuffer)
{
    SegmentDesc plan[Race::kMaxSegments] = {};

    EXPECT_EQ(RaceModel::buildTemplate(Format::Full, true, plan, 30u), 0u)
            << "31 segments must not be written into 30 slots";
    EXPECT_EQ(RaceModel::buildTemplate(Format::Full, false, nullptr, 16u), 0u);
}

TEST(RaceTemplateTest, LabelToleratesAnOutOfRangeStation)
{
    // A corrupt index must not read past kStations.
    EXPECT_EQ(labelOf({ SegmentType::Station, 9u, 9u }), "STATION");
    EXPECT_EQ(labelOf({ SegmentType::Station, 0u, 0u }), "STATION");
}

TEST(RaceTemplateTest, LabelToleratesAZeroSizedBuffer)
{
    char buf[1] = { 'x' };
    RaceModel::label({ SegmentType::Run, 1u, 0u }, buf, 0u);
    EXPECT_EQ(buf[0], 'x') << "nothing should be written";

    RaceModel::label({ SegmentType::Run, 1u, 0u }, nullptr, Race::kMaxLabelLen);
}
