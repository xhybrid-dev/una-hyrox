#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "StateCodec.hpp"

using Knitting::CounterState;
namespace Codec = Knitting::StateCodec;

namespace
{
CounterState make(uint32_t rows, uint32_t set, uint32_t total)
{
    CounterState s;
    s.rows = rows;
    s.set = set;
    s.total = total;
    return s;
}

bool decodeText(const std::string& text, CounterState& out)
{
    return Codec::decode(text.data(), text.size(), out);
}
} // namespace

TEST(StateCodec, EncodesTheDocumentedLayout)
{
    char buf[Codec::kMaxBytes];
    const size_t n = Codec::encode(make(8, 2, 16), buf, sizeof(buf));
    EXPECT_STREQ(buf, "knitting 1\nrows=8\nset=2\ntotal=16\n");
    EXPECT_EQ(n, std::strlen(buf));
}

TEST(StateCodec, RoundTripsTheStartAndTheLimits)
{
    const CounterState cases[] = {
        make(0, 1, 0),
        make(8, 2, 16),
        make(Knitting::kMaxRows, Knitting::kMaxSets, Knitting::kMaxTotal),
    };
    for (const CounterState& c : cases) {
        char buf[Codec::kMaxBytes];
        const size_t n = Codec::encode(c, buf, sizeof(buf));
        ASSERT_GT(n, 0u);
        CounterState back = make(7, 7, 7);
        ASSERT_TRUE(Codec::decode(buf, n, back));
        EXPECT_EQ(back.rows, c.rows);
        EXPECT_EQ(back.set, c.set);
        EXPECT_EQ(back.total, c.total);
    }
}

TEST(StateCodec, TheLongestStateFitsTheStatedBuffer)
{
    char buf[Codec::kMaxBytes];
    EXPECT_GT(Codec::encode(make(4000000000u, 4000000000u, 4000000000u), buf, sizeof(buf)), 0u);
}

TEST(StateCodec, EncodeFailsCleanlyInATooSmallBuffer)
{
    char buf[8];
    std::memset(buf, 'x', sizeof(buf));
    EXPECT_EQ(Codec::encode(make(8, 2, 16), buf, sizeof(buf)), 0u);
    EXPECT_EQ(buf[0], '\0');
    EXPECT_EQ(Codec::encode(make(8, 2, 16), nullptr, 0), 0u);
    // One byte short of the exact fit still fails; the exact fit works.
    const size_t exact = std::strlen("knitting 1\nrows=8\nset=2\ntotal=16\n") + 1;
    char tight[64];
    EXPECT_EQ(Codec::encode(make(8, 2, 16), tight, exact - 1), 0u);
    EXPECT_GT(Codec::encode(make(8, 2, 16), tight, exact), 0u);
}

TEST(StateCodec, RejectsWhatIsNotAStateFile)
{
    CounterState out = make(7, 7, 7);
    const char* bad[] = {
        "",
        "junk",
        "knitting 2\nrows=1\nset=1\ntotal=1\n",           // another version
        "rows=1\nset=1\ntotal=1\n",                        // no header
        "knitting 1\nrows=1\nset=1\n",                     // a missing key
        "knitting 1\nset=1\nrows=1\ntotal=1\n",            // keys out of order
        "knitting 1\nrows=\nset=1\ntotal=1\n",             // empty number
        "knitting 1\nrows=-1\nset=1\ntotal=1\n",           // negative
        "knitting 1\nrows=1.5\nset=1\ntotal=1\n",          // not an integer
        "knitting 1\nrows=1\nset=1\ntotal=1",              // no final newline
        "knitting 1\nrows=1\nset=1\ntotal=1\nextra=2\n",   // trailing garbage
        "knitting 1\r\nrows=1\r\nset=1\r\ntotal=1\r\n",    // edited with CRLF
    };
    for (const char* text : bad) {
        EXPECT_FALSE(decodeText(text, out)) << text;
        EXPECT_EQ(out.rows, 7u) << "left untouched: " << text;
        EXPECT_EQ(out.set, 7u);
        EXPECT_EQ(out.total, 7u);
    }
}

TEST(StateCodec, ATornFileIsRejected)
{
    const std::string whole = "knitting 1\nrows=8\nset=2\ntotal=16\n";
    CounterState out;
    for (size_t cut = 0; cut < whole.size(); ++cut) {
        EXPECT_FALSE(Codec::decode(whole.data(), cut, out)) << "cut at " << cut;
    }
    EXPECT_TRUE(Codec::decode(whole.data(), whole.size(), out));
}

TEST(StateCodec, OutOfRangeValuesAreClamped)
{
    CounterState out;
    ASSERT_TRUE(decodeText("knitting 1\nrows=5000\nset=0\ntotal=99999999999999999999\n", out));
    EXPECT_EQ(out.rows, Knitting::kMaxRows);
    EXPECT_EQ(out.set, 1u);
    EXPECT_EQ(out.total, Knitting::kMaxTotal);
}
