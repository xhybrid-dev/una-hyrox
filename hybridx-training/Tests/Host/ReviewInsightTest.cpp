#include "ReviewInsight.hpp"
#include "ReviewText.hpp"

#include <gtest/gtest.h>

#include <cstring>

using namespace XTraining;
using namespace XTraining::Review;

namespace {

/// A judged session @p day days after 1 November: @p in per cent in band, the rest fast or slow.
ReviewEntry session(int day, uint8_t in, Dir miss, uint8_t feel = 0, bool judged = true)
{
    ReviewEntry e;
    e.date      = Date::addDays(20261101u, day);
    e.kind      = Kind::Intervals;
    e.judgedSec = judged ? 900 : 0;
    if (judged) {
        e.inPct   = in;
        e.fastPct = miss == Dir::Fast ? static_cast<uint8_t>(100 - in) : 0;
        e.gapSecKm = miss == Dir::Fast ? -8 : 8;
    }
    e.feel = feel;
    return e;
}

const ReviewEntry onPace(int day, uint8_t feel = 0) { return session(day, 90, Dir::Slow, feel); }
const ReviewEntry slow(int day, uint8_t feel = 0) { return session(day, 30, Dir::Slow, feel); }
const ReviewEntry fast(int day, uint8_t feel = 0) { return session(day, 30, Dir::Fast, feel); }

constexpr uint8_t kFresh = static_cast<uint8_t>(Feel::Fresh);
constexpr uint8_t kGood = static_cast<uint8_t>(Feel::Good);
constexpr uint8_t kSteady = static_cast<uint8_t>(Feel::Steady);
constexpr uint8_t kTired = static_cast<uint8_t>(Feel::Tired);
constexpr uint8_t kSpent = static_cast<uint8_t>(Feel::Spent);

}  // namespace

TEST(ReviewInsight, NothingToSayWithAnEmptyLog)
{
    ReviewLog l;
    const Insight in = analyse(l);
    EXPECT_EQ(in.judged, 0);
    EXPECT_FALSE(in.streak());
    EXPECT_EQ(lineFor(in), Line::None);
}

TEST(ReviewInsight, OneBadRunIsNeverASuggestion)
{
    ReviewLog l;
    l.record(onPace(1));
    l.record(slow(2));
    const Insight in = analyse(l);
    EXPECT_EQ(in.offRun, 1);
    EXPECT_FALSE(in.streak());
    EXPECT_FALSE(in.offerEase());
    EXPECT_EQ(lineFor(in), Line::None);
}

TEST(ReviewInsight, TwoInARowIsStillNotASuggestion)
{
    ReviewLog l;
    l.record(slow(1));
    l.record(slow(2));
    const Insight in = analyse(l);
    EXPECT_EQ(in.offRun, 2);
    EXPECT_FALSE(in.streak());
    EXPECT_FALSE(in.offerEase());
}

TEST(ReviewInsight, ThreeSlowInARowOffersToHoldThePlan)
{
    ReviewLog l;
    l.record(slow(1));
    l.record(slow(2));
    l.record(slow(4));
    const Insight in = analyse(l);
    EXPECT_EQ(in.offRun, 3);
    EXPECT_TRUE(in.streak());
    EXPECT_EQ(in.dir, Dir::Slow);
    EXPECT_TRUE(in.offerEase());
    EXPECT_EQ(in.newest, Date::addDays(20261101u, 4));
}

TEST(ReviewInsight, ThreeFastInARowOnlySaysSoAndOffersNothing)
{
    ReviewLog l;
    l.record(fast(1));
    l.record(fast(2));
    l.record(fast(3));
    const Insight in = analyse(l);
    EXPECT_TRUE(in.streak());
    EXPECT_EQ(in.dir, Dir::Fast);
    EXPECT_FALSE(in.offerEase());
    EXPECT_EQ(lineFor(in), Line::Fast);
}

TEST(ReviewInsight, ASessionOnPaceBreaksTheRun)
{
    ReviewLog l;
    l.record(slow(1));
    l.record(slow(2));
    l.record(onPace(3));
    l.record(slow(4));
    l.record(slow(5));
    EXPECT_EQ(analyse(l).offRun, 2);
    l.record(slow(6));
    EXPECT_EQ(analyse(l).offRun, 3);
    EXPECT_TRUE(analyse(l).offerEase());
}

TEST(ReviewInsight, AnUnjudgedSessionNeitherBreaksNorExtendsARun)
{
    ReviewLog l;
    l.record(slow(1));
    l.record(session(2, 0, Dir::Slow, kTired, false));   // a treadmill, say
    l.record(slow(3));
    l.record(session(4, 0, Dir::Slow, 0, false));
    l.record(slow(5));
    const Insight in = analyse(l);
    EXPECT_EQ(in.offRun, 3);
    EXPECT_EQ(in.judged, 3);
    EXPECT_TRUE(in.offerEase());
}

TEST(ReviewInsight, TheLineThresholdIsExactlyKOffPacePct)
{
    ReviewLog a;
    a.record(session(1, kOffPacePct, Dir::Slow));       // exactly the threshold: on pace
    a.record(session(2, kOffPacePct, Dir::Slow));
    a.record(session(3, kOffPacePct, Dir::Slow));
    EXPECT_EQ(analyse(a).offRun, 0);
    ReviewLog b;
    b.record(session(1, kOffPacePct - 1, Dir::Slow));
    b.record(session(2, kOffPacePct - 1, Dir::Slow));
    b.record(session(3, kOffPacePct - 1, Dir::Slow));
    EXPECT_EQ(analyse(b).offRun, 3);
}

TEST(ReviewInsight, AnAnsweredSuggestionNeedsThreeNewSessions)
{
    ReviewLog l;
    l.record(slow(1));
    l.record(slow(2));
    l.record(slow(3));
    ASSERT_TRUE(analyse(l).offerEase());
    l.handled = Date::addDays(20261101u, 3);   // declined, or taken, on the third
    EXPECT_FALSE(analyse(l).streak());
    EXPECT_EQ(analyse(l).offRun, 0);
    l.record(slow(4));
    l.record(slow(5));
    EXPECT_FALSE(analyse(l).streak());
    l.record(slow(6));
    EXPECT_TRUE(analyse(l).offerEase());
    EXPECT_EQ(analyse(l).offRun, 3);
}

TEST(ReviewInsight, TheDirectionOfARunIsWhicheverMissWasMore)
{
    ReviewLog l;
    l.record(fast(1));
    l.record(slow(2));
    l.record(slow(3));
    EXPECT_EQ(analyse(l).dir, Dir::Slow);
    ReviewLog m;
    m.record(slow(1));
    m.record(fast(2));
    m.record(fast(3));
    EXPECT_EQ(analyse(m).dir, Dir::Fast);
}

TEST(ReviewInsight, TheLineSaysHowTheRunnerFeltOnTheRun)
{
    ReviewLog tired;
    tired.record(slow(1, kTired));
    tired.record(slow(2, kSpent));
    tired.record(slow(3, kSteady));
    EXPECT_EQ(lineFor(analyse(tired)), Line::SlowTired);

    ReviewLog fresh;
    fresh.record(slow(1, kFresh));
    fresh.record(slow(2, kGood));
    fresh.record(slow(3, kTired));
    EXPECT_EQ(lineFor(analyse(fresh)), Line::SlowFresh);

    ReviewLog none;
    none.record(slow(1));
    none.record(slow(2));
    none.record(slow(3, kSteady));
    EXPECT_EQ(lineFor(analyse(none)), Line::SlowMixed);

    ReviewLog tie;
    tie.record(slow(1, kTired));
    tie.record(slow(2, kFresh));
    tie.record(slow(3));
    EXPECT_EQ(lineFor(analyse(tie)), Line::SlowMixed);
}

TEST(ReviewInsight, OnPaceSaysSoAndNotesATiredRunnerWhoHeldIt)
{
    ReviewLog l;
    l.record(slow(1));
    l.record(onPace(2, kGood));
    EXPECT_EQ(lineFor(analyse(l)), Line::OnTrack);
    l.record(onPace(3, kTired));
    EXPECT_EQ(lineFor(analyse(l)), Line::OnTired);
    l.record(onPace(4));
    EXPECT_EQ(lineFor(analyse(l)), Line::OnTrack);
}

TEST(ReviewInsight, TheFeelAndPaceOfTheRunAreCounted)
{
    ReviewLog l;
    l.record(slow(1, kTired));
    l.record(slow(2));
    l.record(slow(3, kGood));
    l.record(slow(4, kSpent));
    const Insight in = analyse(l);
    EXPECT_EQ(in.offRun, 4);
    EXPECT_EQ(in.asked, 3);
    EXPECT_EQ(in.tired, 2);
    EXPECT_EQ(in.fresh, 1);
}

// ---- The words ---------------------------------------------------------------------------

TEST(ReviewText, TheSavedLineIsShortAndSaysTheDirectionOnlyWhenOffPace)
{
    char b[48];
    ReviewText::savedLine(82, Dir::Fast, b, sizeof(b));
    EXPECT_STREQ(b, "On pace 82%");
    ReviewText::savedLine(100, Dir::None, b, sizeof(b));
    EXPECT_STREQ(b, "On pace 100%");
    ReviewText::savedLine(40, Dir::Fast, b, sizeof(b));
    EXPECT_STREQ(b, "Too fast, 40% on pace");
    ReviewText::savedLine(5, Dir::Slow, b, sizeof(b));
    EXPECT_STREQ(b, "Too slow, 5% on pace");
    ReviewText::savedLine(40, Dir::Slow, nullptr, 0);   // no buffer, no crash
}

TEST(ReviewText, RepsMissesAndFadeInKilometresAndMiles)
{
    char b[48];
    ReviewText::repsLine(5, 6, b, sizeof(b));
    EXPECT_STREQ(b, "5 of 6 reps in band");
    ReviewText::repsLine(1, 1, b, sizeof(b));
    EXPECT_STREQ(b, "");
    ReviewText::missLine(-6, false, b, sizeof(b));
    EXPECT_STREQ(b, "Misses 6 s/km fast");
    ReviewText::missLine(10, true, b, sizeof(b));
    EXPECT_STREQ(b, "Misses 16 s/mi slow");
    ReviewText::missLine(0, false, b, sizeof(b));
    EXPECT_STREQ(b, "");
    ReviewText::fadeLine(7, false, b, sizeof(b));
    EXPECT_STREQ(b, "Later reps 7 s/km slower");
    ReviewText::fadeLine(-5, false, b, sizeof(b));
    EXPECT_STREQ(b, "Later reps 5 s/km faster");
    ReviewText::fadeLine(2, false, b, sizeof(b));
    EXPECT_STREQ(b, "");   // under three: not worth saying
    ReviewText::fadeLine(-100, true, b, sizeof(b));
    EXPECT_STREQ(b, "Later reps 161 s/mi faster");
}

TEST(ReviewText, EveryFeelHasAName)
{
    EXPECT_STREQ(ReviewText::feelName(0), "");
    EXPECT_STREQ(ReviewText::feelName(1), "Fresh");
    EXPECT_STREQ(ReviewText::feelName(5), "Spent");
    EXPECT_STREQ(ReviewText::feelName(6), "");
    EXPECT_STREQ(ReviewText::feelName(255), "");
}

TEST(ReviewText, EveryLineAndQuestionIsBritishEnglishAndBounded)
{
    char b[96];
    const Line lines[] = {Line::OnTrack, Line::OnTired, Line::SlowTired, Line::SlowFresh, Line::SlowMixed, Line::Fast};
    for (Line line : lines) {
        ReviewText::insightLine(line, 3, b, sizeof(b));
        EXPECT_GT(std::strlen(b), 0u);
        EXPECT_LT(std::strlen(b), 40u);   // two lines of the Form page's face
        EXPECT_EQ(std::strstr(b, "color"), nullptr);
    }
    ReviewText::insightLine(Line::None, 3, b, sizeof(b));
    EXPECT_STREQ(b, "");
    // A tiny buffer is cut, never overrun.
    char tiny[8];
    ReviewText::insightLine(Line::SlowFresh, 3, tiny, sizeof(tiny));
    EXPECT_LT(std::strlen(tiny), sizeof(tiny));
    for (uint8_t d = 0; d < 6; ++d) {
        EXPECT_GT(std::strlen(ReviewText::easeDenied(d)), 0u);
    }
}
