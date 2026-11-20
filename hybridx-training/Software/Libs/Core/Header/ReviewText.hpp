/**
 ******************************************************************************
 * @file    ReviewText.hpp
 * @brief   The words of the session review (TP8), British English. Header-only,
 *          integers only: the GUI process uses it as the service does. The
 *          GUI wraps and fits them to the round face; the simulator's audit
 *          measures the widest of each.
 ******************************************************************************
 */

#ifndef XTRAINING_REVIEW_TEXT_HPP
#define XTRAINING_REVIEW_TEXT_HPP

#include "ReviewInsight.hpp"
#include "SessionReview.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace XTraining {
namespace ReviewText {

/// "Fresh" ... "Spent"; "" for not asked.
inline const char* feelName(uint8_t feel)
{
    static const char* const kNames[Review::kFeelCount] = {"", "Fresh", "Good", "Steady", "Tired", "Spent"};
    return feel < Review::kFeelCount ? kNames[feel] : "";
}

inline const char* feelQuestion() { return "How do you feel?"; }

/// A gap in s/km as the runner's units have it: s/km, or s/mi (1.609 km to the mile).
inline uint32_t gapInUnits(uint32_t secKm, bool imperial) { return imperial ? (secKm * 1609u + 500u) / 1000u : secKm; }
inline const char* gapUnit(bool imperial) { return imperial ? "s/mi" : "s/km"; }

/// The Saved screen's line: "On pace 82%", or "Too fast, 40% on pace" / "Too slow, 40% on pace".
inline void savedLine(uint8_t inPct, Review::Dir dir, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return;
    }
    if (inPct >= Review::kOffPacePct || dir == Review::Dir::None) {
        std::snprintf(buf, cap, "On pace %u%%", static_cast<unsigned>(inPct));
    } else {
        std::snprintf(buf, cap, "Too %s, %u%% on pace", dir == Review::Dir::Fast ? "fast" : "slow",
                      static_cast<unsigned>(inPct));
    }
}

/// "5 of 6 reps in band"; "" with fewer than two.
inline void repsLine(uint8_t unitsIn, uint8_t units, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return;
    }
    if (units < 2) {
        buf[0] = '\0';
        return;
    }
    std::snprintf(buf, cap, "%u of %u reps in band", static_cast<unsigned>(unitsIn), static_cast<unsigned>(units));
}

/// "Misses 6 s/km fast" / "Misses 6 s/km slow"; "" for none.
inline void missLine(int16_t gapSecKm, bool imperial, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return;
    }
    if (gapSecKm == 0) {
        buf[0] = '\0';
        return;
    }
    const uint32_t mag = gapSecKm < 0 ? static_cast<uint32_t>(-static_cast<int32_t>(gapSecKm)) : static_cast<uint32_t>(gapSecKm);
    std::snprintf(buf, cap, "Misses %u %s %s", static_cast<unsigned>(gapInUnits(mag, imperial)), gapUnit(imperial),
                  gapSecKm < 0 ? "fast" : "slow");
}

/// "Later reps 6 s/km slower" / "faster"; "" when the fade is under kFadeShowSecKm.
inline void fadeLine(int8_t fadeSecKm, bool imperial, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return;
    }
    const int32_t f = fadeSecKm;
    const uint32_t mag = static_cast<uint32_t>(f < 0 ? -f : f);
    if (mag < static_cast<uint32_t>(Review::kFadeShowSecKm)) {
        buf[0] = '\0';
        return;
    }
    std::snprintf(buf, cap, "Later reps %u %s %s", static_cast<unsigned>(gapInUnits(mag, imperial)), gapUnit(imperial),
                  f > 0 ? "slower" : "faster");
}

/// The Form page's one line, from the pattern; "" for none. @p n is how many sessions in the run.
inline void insightLine(Review::Line line, uint8_t n, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return;
    }
    buf[0] = '\0';
    switch (line) {
    case Review::Line::OnTrack:   std::snprintf(buf, cap, "On pace. Keep it steady."); break;
    case Review::Line::OnTired:   std::snprintf(buf, cap, "Held the pace while tired."); break;
    case Review::Line::SlowTired: std::snprintf(buf, cap, "%u slow and tired. Rest up.", static_cast<unsigned>(n)); break;
    case Review::Line::SlowFresh: std::snprintf(buf, cap, "Slow yet fresh: paces too hard?"); break;
    case Review::Line::SlowMixed: std::snprintf(buf, cap, "%u in a row too slow.", static_cast<unsigned>(n)); break;
    case Review::Line::Fast:      std::snprintf(buf, cap, "%u too fast. Hold the pace.", static_cast<unsigned>(n)); break;
    case Review::Line::None:      break;
    }
}

/// Why the repeat was not made (PlanRules::Settled::Denied as a number, 1 on), in few words for Today's italic line.
inline const char* easeDenied(uint8_t denied)
{
    switch (denied) {
    case 1:  return "too near the race";
    case 2:  return "plan is long enough";
    case 3:  return "last week of plan";
    default: return "already repeated";
    }
}

}  // namespace ReviewText
}  // namespace XTraining

#endif  // XTRAINING_REVIEW_TEXT_HPP
