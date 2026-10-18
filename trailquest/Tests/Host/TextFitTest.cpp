// Fitting text to the round face: the chord at a row, characters per row, and
// word-wrapping into fixed buffers.

#include <gtest/gtest.h>

#include <cstring>

#include "TextFit.hpp"

using namespace TrailQuest::TextFit;

TEST(TextFit, TheChordIsWidestInTheMiddleAndZeroOffTheFace)
{
    EXPECT_EQ(isqrt(0), 0u);
    EXPECT_EQ(isqrt(15), 3u);
    EXPECT_EQ(isqrt(16), 4u);
    EXPECT_EQ(isqrt(57600), 240u);
    EXPECT_EQ(chordAt(-1), 0);
    EXPECT_EQ(chordAt(240), 0);
    EXPECT_GE(chordAt(119), 239);
    EXPECT_EQ(chordAt(119), chordAt(120));   // symmetric about the middle
    EXPECT_EQ(chordAt(10), chordAt(229));
    // A worked value: row 205 is 85.5 px below the centre, so the chord is
    // 2 x sqrt(120^2 - 85.5^2) = 168.4 px.
    EXPECT_EQ(chordAt(205), 168);
    for (int32_t y = 1; y < 120; ++y) {
        EXPECT_GE(chordAt(y), chordAt(y - 1)) << y;
    }
}

TEST(TextFit, ColumnsFollowTheChord)
{
    // Row 28's band (28-43) is 155 px at its top: (155 - 8) / 16 = 9.
    EXPECT_EQ(colsAt(28), 9u);
    EXPECT_EQ(colsAt(112), kMaxCols);
    EXPECT_EQ(colsAt(230), 0u);
    // Split at the middle, a corner label at row 56 has (204 / 2 - 4 - 4) / 16 = 5.
    EXPECT_EQ(halfColsAt(56), 5u);
    EXPECT_EQ(colsOver(74, 5, 17), 13u);
}

TEST(TextFit, WrapBreaksBetweenWords)
{
    Lines l;
    wrap(l, "Sits on you at mile 20. Does not apologise.", 13);
    ASSERT_TRUE(l.fits);
    ASSERT_EQ(l.count, 4u);
    EXPECT_STREQ(l.text[0], "Sits on you");
    EXPECT_STREQ(l.text[1], "at mile 20.");
    EXPECT_STREQ(l.text[2], "Does not");
    EXPECT_STREQ(l.text[3], "apologise.");
}

TEST(TextFit, AWordLongerThanALineIsCutAndSaysSo)
{
    Lines l;
    wrap(l, "Unbelievably", 5);
    EXPECT_FALSE(l.fits);
    ASSERT_EQ(l.count, 3u);
    EXPECT_STREQ(l.text[0], "Unbel");
    EXPECT_STREQ(l.text[1], "ievab");
    EXPECT_STREQ(l.text[2], "ly");
}

TEST(TextFit, RunningOutOfLinesSaysSo)
{
    Lines l;
    wrap(l, "one two three four", 5, 2);
    EXPECT_FALSE(l.fits);
    EXPECT_EQ(l.count, 2u);
    EXPECT_STREQ(l.text[0], "one");
    EXPECT_STREQ(l.text[1], "two");
}

TEST(TextFit, ANewlineStartsALineAndBlocksAppend)
{
    Lines l;
    addLine(l, "Title", 13);
    wrap(l, "a b\nc", 13);
    ASSERT_EQ(l.count, 3u);
    EXPECT_STREQ(l.text[0], "Title");
    EXPECT_STREQ(l.text[1], "a b");
    EXPECT_STREQ(l.text[2], "c");
    EXPECT_TRUE(l.fits);
    l.clear();
    EXPECT_EQ(l.count, 0u);
    EXPECT_TRUE(l.fits);
}

TEST(TextFit, NeverWritesPastItsBuffers)
{
    Lines l;
    for (int i = 0; i < 20; ++i) {
        addLine(l, "a line far longer than any row could ever hold", 40);
    }
    EXPECT_EQ(l.count, kMaxLines);
    EXPECT_FALSE(l.fits);
    for (const auto& t : l.text) {
        EXPECT_LE(std::strlen(t), static_cast<size_t>(kMaxCols));
    }
    Lines w;
    wrap(w, nullptr, 10);
    wrap(w, "text", 0);
    EXPECT_EQ(w.count, 0u);
}
