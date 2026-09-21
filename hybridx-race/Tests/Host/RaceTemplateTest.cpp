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
