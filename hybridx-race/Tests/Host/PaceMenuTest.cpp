#include <gtest/gtest.h>

#include "PaceMenu.hpp"
#include "PacePlan.hpp"

#include <string>

using namespace Race;
using namespace Race::PaceMenu;

namespace
{

std::string tipText(PaceMode mode, uint8_t status, uint16_t targetMin = 0u)
{
    char buf[16];
    tip(buf, sizeof(buf), mode, status, targetMin);
    return buf;
}

}  // namespace

// The GUI cannot include PacePlan.hpp, so it names the status values itself. If the enum is ever
// reordered this fails, instead of the menu quietly offering plans that do not exist.
TEST(PaceMenu, StatusValuesMatchThePlansEnum)
{
    EXPECT_EQ(kStatusReady, static_cast<uint8_t>(PacePlan::Status::Ready));
    EXPECT_EQ(kStatusNoLastRace, static_cast<uint8_t>(PacePlan::Status::NoLastRace));
    EXPECT_EQ(kStatusNoMatch, static_cast<uint8_t>(PacePlan::Status::NoMatch));
    EXPECT_EQ(kStatusNoTarget, static_cast<uint8_t>(PacePlan::Status::NoTarget));
}

TEST(PaceMenu, OffIsAlwaysUsableAndAnUnknownStatusIsNot)
{
    EXPECT_TRUE(usable(PaceMode::Off, 0));
    EXPECT_TRUE(usable(PaceMode::Off, kStatusNoLastRace));
    EXPECT_TRUE(usable(PaceMode::LastRace, kStatusReady));
    EXPECT_FALSE(usable(PaceMode::LastRace, 0));       // no settings message yet
    EXPECT_FALSE(usable(PaceMode::Target, kStatusNoTarget));
}

TEST(PaceMenu, PressingCyclesOffLastRaceTargetOff)
{
    const uint8_t all[kPaceModeCount] = {0, kStatusReady, kStatusReady};
    EXPECT_EQ(nextMode(PaceMode::Off, all), PaceMode::LastRace);
    EXPECT_EQ(nextMode(PaceMode::LastRace, all), PaceMode::Target);
    EXPECT_EQ(nextMode(PaceMode::Target, all), PaceMode::Off);
}

TEST(PaceMenu, UnusableModesAreSkipped)
{
    // Last race usable, Target not (no target time set).
    const uint8_t noTarget[kPaceModeCount] = {0, kStatusReady, kStatusNoTarget};
    EXPECT_EQ(nextMode(PaceMode::Off, noTarget), PaceMode::LastRace);
    EXPECT_EQ(nextMode(PaceMode::LastRace, noTarget), PaceMode::Off);

    // Nothing but Off: a press stays on Off rather than looping.
    const uint8_t none[kPaceModeCount] = {0, kStatusNoLastRace, kStatusNoLastRace};
    EXPECT_EQ(nextMode(PaceMode::Off, none), PaceMode::Off);
}

TEST(PaceMenu, AModeThatBecameUnusableStillLeavesToAUsableOne)
{
    // Selected Last race, then the format changed and it no longer matches.
    const uint8_t gone[kPaceModeCount] = {0, kStatusNoMatch, kStatusNoMatch};
    EXPECT_EQ(nextMode(PaceMode::LastRace, gone), PaceMode::Off);
    EXPECT_TRUE(usable(nextMode(PaceMode::LastRace, gone), gone[0]));
}

TEST(PaceMenu, TheHintNamesTheModeOrTheReason)
{
    EXPECT_EQ(tipText(PaceMode::Off, 0), "Off");
    EXPECT_EQ(tipText(PaceMode::LastRace, kStatusReady), "Last race");
    EXPECT_EQ(tipText(PaceMode::Target, kStatusReady, 105), "Target 1:45");
    EXPECT_EQ(tipText(PaceMode::Target, kStatusReady, 240), "Target 4:00");
    EXPECT_EQ(tipText(PaceMode::LastRace, kStatusNoLastRace), "No saved race");
    EXPECT_EQ(tipText(PaceMode::LastRace, kStatusNoMatch), "Doesn't match");
    EXPECT_EQ(tipText(PaceMode::Target, kStatusNoTarget), "No target set");
}

TEST(PaceMenu, EveryHintFitsTheMenusHintLine)
{
    // The longest hint the hint font holds in the wheel is about 13 characters ("Half: 1 to 4" is
    // 12, "Target 4:00" is 11).
    const uint8_t statuses[] = {0, kStatusReady, kStatusNoLastRace, kStatusNoMatch, kStatusNoTarget};
    for (uint8_t m = 0; m < kPaceModeCount; ++m) {
        for (uint8_t s : statuses) {
            EXPECT_LE(tipText(static_cast<PaceMode>(m), s, 240).size(), 13u) << int(m) << "/" << int(s);
        }
    }
}

TEST(PaceMenu, TheTargetStepsInFivesFromAnHour)
{
    EXPECT_EQ(nextTargetMin(0), 60);
    EXPECT_EQ(nextTargetMin(60), 65);
    EXPECT_EQ(nextTargetMin(95), 100);
    EXPECT_EQ(nextTargetMin(235), 240);
    EXPECT_EQ(nextTargetMin(240), 0);   // wraps to Off
}

TEST(PaceMenu, APhoneValueBetweenStepsMovesUpToTheNextStep)
{
    EXPECT_EQ(nextTargetMin(97), 100);
    EXPECT_EQ(nextTargetMin(61), 65);
    EXPECT_EQ(nextTargetMin(239), 240);
    // Below an hour (a half race target from the phone): the first press goes to the first step.
    EXPECT_EQ(nextTargetMin(45), 60);
    EXPECT_EQ(nextTargetMin(1), 60);
}

TEST(PaceMenu, EveryPressFromEveryValueLandsInRangeAndEventuallyReturnsToOff)
{
    for (uint16_t start = 0; start <= kTargetMaxMin; ++start) {
        uint16_t v = start;
        int presses = 0;
        do {
            v = nextTargetMin(v);
            ++presses;
            ASSERT_LE(v, kTargetMaxMin) << start;
            ASSERT_LT(presses, 100) << "does not return to Off from " << start;
        } while (v != 0);
    }
}

TEST(PaceMenu, TheTargetReadsAsHoursAndMinutes)
{
    char buf[12];
    targetText(buf, sizeof(buf), 0);
    EXPECT_STREQ(buf, "Off");
    targetText(buf, sizeof(buf), 105);
    EXPECT_STREQ(buf, "1:45");
    targetText(buf, sizeof(buf), 60);
    EXPECT_STREQ(buf, "1:00");
}

TEST(PaceMenu, NullAndTinyBuffersAreSafe)
{
    tip(nullptr, 0, PaceMode::Off, 0, 0);
    targetText(nullptr, 0, 1);
    char tiny[3] = {'x', 'x', 'x'};
    tip(tiny, 2, PaceMode::LastRace, kStatusReady, 0);   // snprintf truncates to 1 char + NUL
    EXPECT_EQ(tiny[2], 'x');
}
