// RecordSpool: per-second records wait for their segment to close, then are
// written with the distance ramped across them. The properties that matter to a
// consumer app are that distance never goes backwards, that every record is
// written exactly once, and that each segment ends on exactly its distance.

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "RecordSpool.hpp"

namespace {

struct Rec {
    uint32_t id;
};

using Spool = Race::RecordSpool<Rec, 8>;

struct Out {
    uint32_t id;
    uint32_t cm;
};

struct Collector {
    std::vector<Out> *out;
    void operator()(const Rec &r, uint32_t cm) { out->push_back({r.id, cm}); }
};

}  // namespace

TEST(RecordSpool, RampsEvenlyAndEndsOnTheTarget)
{
    Spool s;
    std::vector<Out> out;
    Collector sink{&out};

    for (uint32_t i = 0; i < 4; ++i) {
        s.add({i}, sink);
    }
    EXPECT_TRUE(out.empty()) << "nothing is written until the segment closes";

    s.close(40000u, sink);  // a 400 m run
    ASSERT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0].cm, 10000u);
    EXPECT_EQ(out[1].cm, 20000u);
    EXPECT_EQ(out[2].cm, 30000u);
    EXPECT_EQ(out[3].cm, 40000u);
    EXPECT_EQ(s.size(), 0u);
    EXPECT_EQ(s.writtenCm(), 40000u);
}

TEST(RecordSpool, NextSegmentRampsFromWhereTheLastEnded)
{
    Spool s;
    std::vector<Out> out;
    Collector sink{&out};

    for (uint32_t i = 0; i < 2; ++i) s.add({i}, sink);
    s.close(40000u, sink);
    for (uint32_t i = 2; i < 4; ++i) s.add({i}, sink);
    s.close(40000u, sink);  // a Roxzone: no metres

    ASSERT_EQ(out.size(), 4u);
    EXPECT_EQ(out[1].cm, 40000u);
    EXPECT_EQ(out[2].cm, 40000u);
    EXPECT_EQ(out[3].cm, 40000u);
}

TEST(RecordSpool, WritesEveryRecordOnceInOrder)
{
    Spool s;
    std::vector<Out> out;
    Collector sink{&out};

    // Three times the capacity, so the overflow path runs.
    for (uint32_t i = 0; i < 24; ++i) s.add({i}, sink);
    s.close(100000u, sink);

    ASSERT_EQ(out.size(), 24u);
    for (uint32_t i = 0; i < 24; ++i) {
        EXPECT_EQ(out[i].id, i);
    }
}

TEST(RecordSpool, OverflowWritesTheOldestFlat)
{
    Spool s;
    std::vector<Out> out;
    Collector sink{&out};

    for (uint32_t i = 0; i < 10; ++i) s.add({i}, sink);
    ASSERT_EQ(out.size(), 2u);  // 8 held, 2 pushed out
    EXPECT_EQ(out[0].cm, 0u);
    EXPECT_EQ(out[1].cm, 0u);
}

TEST(RecordSpool, DistanceNeverGoesBackwards)
{
    Spool s;
    std::vector<Out> out;
    Collector sink{&out};

    for (uint32_t i = 0; i < 3; ++i) s.add({i}, sink);
    s.close(100000u, sink);  // segment 1 closed at 1000 m

    // The athlete undoes the split, then closes it again a little later. The
    // model's total is still 1000 m; the file has already been told 1000 m.
    for (uint32_t i = 3; i < 6; ++i) s.add({i}, sink);
    s.close(100000u, sink);

    // And an undo of two, so the model's total is now lower than what was written.
    for (uint32_t i = 6; i < 9; ++i) s.add({i}, sink);
    s.close(50000u, sink);

    uint32_t last = 0;
    for (const Out &o : out) {
        EXPECT_GE(o.cm, last);
        last = o.cm;
    }
    EXPECT_EQ(s.writtenCm(), 100000u);
}

TEST(RecordSpool, ASegmentWithNoRecordsStillAdvancesTheDistance)
{
    Spool s;
    std::vector<Out> out;
    Collector sink{&out};

    s.close(40000u, sink);  // split between two ticks
    EXPECT_TRUE(out.empty());
    EXPECT_EQ(s.writtenCm(), 40000u);

    s.add({0}, sink);
    s.close(40000u, sink);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].cm, 40000u);
}

TEST(RecordSpool, LargeSpansDoNotOverflow)
{
    // 10 km in centimetres, over 8 records: the product exceeds 32 bits.
    Spool s;
    std::vector<Out> out;
    Collector sink{&out};

    for (uint32_t i = 0; i < 8; ++i) s.add({i}, sink);
    s.close(1000000u * 4u, sink);
    EXPECT_EQ(out.back().cm, 4000000u);
}

TEST(RecordSpool, ResetStartsAgain)
{
    Spool s;
    std::vector<Out> out;
    Collector sink{&out};

    s.add({0}, sink);
    s.close(5000u, sink);
    s.reset();
    EXPECT_EQ(s.writtenCm(), 0u);
    EXPECT_EQ(s.size(), 0u);
}
