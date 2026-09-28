/**
 * @file    TextFoldTest.cpp
 * @brief   Trail::Text::asciiFold: route names as the watch's ASCII fonts can show them.
 */

#include <gtest/gtest.h>

#include <string>

#include "TextFold.hpp"

using Trail::Text::asciiFold;

namespace
{

std::string fold(const char* in, size_t cap = 64)
{
    char out[64];
    asciiFold(out, cap, in);
    return out;
}

} // namespace

TEST(TextFold, AsciiIsUnchanged)
{
    EXPECT_EQ(fold("Stadium 3 laps"), "Stadium 3 laps");
    EXPECT_EQ(fold("~!@#$%^&*()_+{}|:<>?"), "~!@#$%^&*()_+{}|:<>?");
    EXPECT_EQ(fold(""), "");
    EXPECT_EQ(fold(nullptr), "");
}

TEST(TextFold, WelshAndOtherAccentsLoseTheirAccent)
{
    EXPECT_EQ(fold("Ŵyl y Ffynnon, Tŷ Mawr, Llanfair Caereinion â"), "Wyl y Ffynnon, Ty Mawr, Llanfair Caereinion a");
    EXPECT_EQ(fold("Beinn Eighe à Sgùrr"), "Beinn Eighe a Sgurr");
    EXPECT_EQ(fold("Crêpe café, Zürich, Łódź, Ørsta"), "Crepe cafe, Zurich, Lodz, Orsta");
    EXPECT_EQ(fold("Straße, Æbeltoft, Œuvre"), "Strasse, AEbeltoft, OEuvre");
}

TEST(TextFold, PunctuationBecomesItsAsciiCousin)
{
    EXPECT_EQ(fold("Llyn y Fan Fach – Picws Du"), "Llyn y Fan Fach - Picws Du");
    EXPECT_EQ(fold("Jon’s “long” loop…"), "Jon's \"long\" loop...");
    EXPECT_EQ(fold("10\xC2\xA0km"), "10 km");   // no-break space
}

TEST(TextFold, AnythingElseIsAQuestionMark)
{
    EXPECT_EQ(fold("Run \xF0\x9F\x8F\x83 club"), "Run ? club");   // an emoji (4 bytes)
    EXPECT_EQ(fold("\xE6\x9D\xB1\xE4\xBA\xAC"), "??");            // two CJK characters
    EXPECT_EQ(fold("a\x80" "b"), "a?b");                          // a stray continuation byte
    EXPECT_EQ(fold("a\xFF" "b"), "a?b");                          // never UTF-8
}

TEST(TextFold, AMalformedSequenceKeepsTheByteAfterIt)
{
    // A lead byte promising two more, then plain ASCII: the lead is '?', the rest kept.
    EXPECT_EQ(fold("a\xE2" "bc"), "a?bc");
}

TEST(TextFold, ControlCharactersBecomeSpaces)
{
    EXPECT_EQ(fold("Line\tone\nLine two"), "Line one Line two");
}

TEST(TextFold, ACharacterCutShortAtTheEndIsDropped)
{
    // "Tŷ" cut after the first byte of ŷ (C5 B7), as a name truncated to its buffer.
    EXPECT_EQ(fold("T\xC5"), "T");
    EXPECT_EQ(fold("dash \xE2\x80"), "dash ");
}

TEST(TextFold, TheOutputIsCutToFitAndTerminated)
{
    char out[6];
    EXPECT_EQ(asciiFold(out, sizeof(out), "Pen y Fan"), 5u);
    EXPECT_STREQ(out, "Pen y");

    // A fold to several letters stops at the edge too.
    EXPECT_EQ(asciiFold(out, sizeof(out), "abcd\xE2\x80\xA6"), 5u);   // "abcd..."
    EXPECT_STREQ(out, "abcd.");

    char one[1];
    EXPECT_EQ(asciiFold(one, sizeof(one), "x"), 0u);
    EXPECT_STREQ(one, "");

    EXPECT_EQ(asciiFold(nullptr, 8, "x"), 0u);
    EXPECT_EQ(asciiFold(out, 0, "x"), 0u);
}
