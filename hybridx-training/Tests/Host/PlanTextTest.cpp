#include "PlanText.hpp"

#include <gtest/gtest.h>

#include <cstring>

using namespace XTraining;

TEST(PlanText, EveryKindHasWords)
{
    for (uint8_t k = 0; k < kKindCount; ++k) {
        const Kind kind = static_cast<Kind>(k);
        EXPECT_GT(std::strlen(PlanText::name(kind)), 0u) << int(k);
        EXPECT_GT(std::strlen(PlanText::purpose(kind)), 0u) << int(k);
        EXPECT_GT(std::strlen(PlanText::shortName(kind)), 0u) << int(k);
        EXPECT_LE(std::strlen(PlanText::shortName(kind)), 10u) << PlanText::shortName(kind);
        // ASCII only (REVIEW.md E7).
        for (const char* s : {PlanText::name(kind), PlanText::purpose(kind), PlanText::shortName(kind)}) {
            for (const char* c = s; *c != '\0'; ++c) {
                EXPECT_LT(static_cast<unsigned char>(*c), 0x80u) << s;
            }
        }
    }
}

TEST(PlanText, What)
{
    char b[48];
    PlanText::what(Session{Kind::Intervals, 4, 240, 0, 0}, 0, b, sizeof(b));
    EXPECT_STREQ(b, "4 x 4 min");
    PlanText::what(Session{Kind::Intervals, 4, 150, 0, 0}, 0, b, sizeof(b));
    EXPECT_STREQ(b, "4 x 2:30");
    PlanText::what(Session{Kind::Hills, 8, 45, 0, 0}, 0, b, sizeof(b));
    EXPECT_STREQ(b, "8 x 45 s");
    PlanText::what(Session{Kind::LongFinish, 0, 100, 20, 0}, 0, b, sizeof(b));
    EXPECT_STREQ(b, "100 min, last 20 at half pace");
    PlanText::what(Session{Kind::ThirtyThirty, 2, 8, 0, 0}, 0, b, sizeof(b));
    EXPECT_STREQ(b, "2 x 8 x 30/30");
    PlanText::what(Session{Kind::Reps, 6, 400, 0, 0}, 0, b, sizeof(b));
    EXPECT_STREQ(b, "6 x 400 m");
    PlanText::what(Session{Kind::Race}, 2109750u, b, sizeof(b));
    EXPECT_STREQ(b, "21.1 km");
    PlanText::what(Session{Kind::Race}, 1000000u, b, sizeof(b));
    EXPECT_STREQ(b, "10 km");
    PlanText::what(Session{Kind::Rest}, 0, b, sizeof(b));
    EXPECT_STREQ(b, "");
    // Never past the buffer.
    char small[5];
    PlanText::what(Session{Kind::LongFinish, 0, 100, 20, 0}, 0, small, sizeof(small));
    EXPECT_EQ(std::strlen(small), 4u);
}
