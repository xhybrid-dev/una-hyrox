// The feedback table must fit what the kernel accepts, or the pattern is
// silently cut short on the watch.

#include <gtest/gtest.h>

#include "Feedback.hpp"

using namespace TrailQuest;

TEST(Feedback, EveryPatternFitsTheKernelsLimits)
{
    for (uint8_t i = 0; i < static_cast<uint8_t>(FxId::Count); ++i) {
        const FxPattern& p = fxPattern(static_cast<FxId>(i));
        ASSERT_NE(p.name, nullptr);
        EXPECT_GE(p.vibroCount, 1u) << p.name;
        EXPECT_LE(p.vibroCount, kMaxVibroNotes) << p.name;
        EXPECT_GE(p.buzzerCount, 1u) << p.name;
        EXPECT_LE(p.buzzerCount, kMaxBuzzerNotes) << p.name;
    }
}

TEST(Feedback, VibroNotesUseKnownEffectsAndPauseOnlyWhenSilent)
{
    for (uint8_t i = 0; i < static_cast<uint8_t>(FxId::Count); ++i) {
        const FxPattern& p = fxPattern(static_cast<FxId>(i));
        for (uint8_t n = 0; n < p.vibroCount; ++n) {
            const VibroNote& v = p.vibro[n];
            EXPECT_TRUE(isKnownVibroEffect(v.effect)) << p.name << " note " << int(n);
            if (v.effect != 0) {
                EXPECT_EQ(v.pauseMs, 0u) << p.name << " note " << int(n);
            } else {
                EXPECT_GT(v.pauseMs, 0u) << p.name << " note " << int(n);
            }
        }
    }
}

TEST(Feedback, BuzzerNotesUseSupportedVolumesAndHaveALength)
{
    for (uint8_t i = 0; i < static_cast<uint8_t>(FxId::Count); ++i) {
        const FxPattern& p = fxPattern(static_cast<FxId>(i));
        for (uint8_t n = 0; n < p.buzzerCount; ++n) {
            EXPECT_TRUE(isBuzzerVolume(p.buzzer[n].volume)) << p.name << " note " << int(n);
            EXPECT_GT(p.buzzer[n].ms, 0u) << p.name << " note " << int(n);
            // A sound has a pitch the buzzer plays (500-20000 Hz); a rest has none.
            EXPECT_TRUE(isBuzzerHz(p.buzzer[n].hz)) << p.name << " note " << int(n);
            if (p.buzzer[n].volume == 0) {
                EXPECT_EQ(p.buzzer[n].hz, 0u) << p.name << " rest " << int(n);
            } else {
                EXPECT_GE(p.buzzer[n].hz, kMinBuzzerHz) << p.name << " note " << int(n);
            }
        }
    }
}

TEST(Feedback, PatternsAreDistinctFromEachOther)
{
    // Two moments that feel the same cannot be told apart blind.
    for (uint8_t a = 0; a < static_cast<uint8_t>(FxId::Count); ++a) {
        for (uint8_t b = a + 1; b < static_cast<uint8_t>(FxId::Count); ++b) {
            const FxPattern& x = fxPattern(static_cast<FxId>(a));
            const FxPattern& y = fxPattern(static_cast<FxId>(b));
            bool same = x.vibroCount == y.vibroCount && x.buzzerCount == y.buzzerCount;
            for (uint8_t n = 0; same && n < x.vibroCount; ++n) {
                same = x.vibro[n].effect == y.vibro[n].effect && x.vibro[n].pauseMs == y.vibro[n].pauseMs;
            }
            for (uint8_t n = 0; same && n < x.buzzerCount; ++n) {
                same = x.buzzer[n].ms == y.buzzer[n].ms && x.buzzer[n].volume == y.buzzer[n].volume
                    && x.buzzer[n].hz == y.buzzer[n].hz;
            }
            EXPECT_FALSE(same) << x.name << " and " << y.name;
        }
    }
}

TEST(Feedback, AnOutOfRangeIdGivesTheMissPattern)
{
    EXPECT_STREQ(fxPattern(static_cast<FxId>(200)).name, "Miss");
}

TEST(Feedback, KnownEffectsAndVolumes)
{
    EXPECT_TRUE(isKnownVibroEffect(47));
    EXPECT_FALSE(isKnownVibroEffect(2));
    EXPECT_TRUE(isBuzzerVolume(66));
    EXPECT_FALSE(isBuzzerVolume(50));
}

namespace
{
uint16_t firstPitch(const FxPattern& p)
{
    for (uint8_t n = 0; n < p.buzzerCount; ++n) {
        if (p.buzzer[n].hz != 0) {
            return p.buzzer[n].hz;
        }
    }
    return 0;
}

uint16_t lastPitch(const FxPattern& p)
{
    for (uint8_t n = p.buzzerCount; n > 0; --n) {
        if (p.buzzer[n - 1].hz != 0) {
            return p.buzzer[n - 1].hz;
        }
    }
    return 0;
}

uint32_t lengthMs(const FxPattern& p)
{
    uint32_t t = 0;
    for (uint8_t n = 0; n < p.buzzerCount; ++n) {
        t += p.buzzer[n].ms;
    }
    return t;
}
} // namespace

TEST(Feedback, GoodNewsRisesAndBadNewsFalls)
{
    for (FxId good : { FxId::Perfect, FxId::LevelUp, FxId::Win, FxId::StageClear, FxId::Finale, FxId::Found, FxId::Fuel, FxId::Hit }) {
        const FxPattern& p = fxPattern(good);
        EXPECT_GT(lastPitch(p), firstPitch(p)) << p.name;
    }
    for (FxId bad : { FxId::Miss, FxId::Lose }) {
        const FxPattern& p = fxPattern(bad);
        EXPECT_LT(lastPitch(p), firstPitch(p)) << p.name;
    }
}

TEST(Feedback, NoJingleOutstaysItsWelcome)
{
    for (uint8_t i = 0; i < static_cast<uint8_t>(FxId::Count); ++i) {
        const FxPattern& p = fxPattern(static_cast<FxId>(i));
        EXPECT_LE(lengthMs(p), p.buzzerCount > 8 ? 1500u : 1000u) << p.name;
    }
    // The finale is the longest, as befits it.
    for (uint8_t i = 0; i < static_cast<uint8_t>(FxId::Count); ++i) {
        if (static_cast<FxId>(i) != FxId::Finale) {
            EXPECT_LT(lengthMs(fxPattern(static_cast<FxId>(i))), lengthMs(fxPattern(FxId::Finale)));
        }
    }
}

TEST(Feedback, OutOfRangePitchesAreRefused)
{
    EXPECT_TRUE(isBuzzerHz(0));
    EXPECT_TRUE(isBuzzerHz(500));
    EXPECT_TRUE(isBuzzerHz(20000));
    EXPECT_FALSE(isBuzzerHz(499));
    EXPECT_FALSE(isBuzzerHz(20001));
}
