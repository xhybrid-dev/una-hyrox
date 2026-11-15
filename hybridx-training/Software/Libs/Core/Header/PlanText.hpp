/**
 ******************************************************************************
 * @file    PlanText.hpp
 * @brief   The words for sessions: names, short names, purpose lines and the
 *          "what" line ("4 x 4 min"). Header-only (the GUI uses it).
 *
 * British English, ASCII only: the watch's fonts and RunLVGL's text folding
 * draw ASCII safely, and "x" stands for the multiplication sign until the
 * text-fit audit (TP3) shows a glyph (REVIEW.md E7).
 ******************************************************************************
 */

#ifndef XTRAINING_PLAN_TEXT_HPP
#define XTRAINING_PLAN_TEXT_HPP

#include "PlanTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace XTraining {
namespace PlanText {

/// The session's name: "Intervals", "Long run, fast finish".
inline const char* name(Kind k)
{
    switch (k) {
    case Kind::Rest:         return "Rest day";
    case Kind::Easy:         return "Easy run";
    case Kind::EasyStrides:  return "Easy run, strides";
    case Kind::Long:         return "Long run";
    case Kind::LongFinish:   return "Long run, fast finish";
    case Kind::Tempo:        return "Tempo run";
    case Kind::Cruise:       return "Cruise intervals";
    case Kind::Intervals:    return "Intervals";
    case Kind::ThirtyThirty: return "30-30s";
    case Kind::Hills:        return "Hill reps";
    case Kind::Reps:         return "Repetitions";
    case Kind::Pace10k:      return "10K pace";
    case Kind::PaceHalf:     return "Half pace";
    case Kind::TimeTrial5k:  return "5K time trial";
    case Kind::TimeTrial10k: return "10K time trial";
    case Kind::Shakeout:     return "Shake-out";
    case Kind::Race:         return "Race";
    }
    return "";
}

/// A short name for the week list and the glance (at most 10 characters).
inline const char* shortName(Kind k)
{
    switch (k) {
    case Kind::Rest:         return "Rest";
    case Kind::Easy:         return "Easy";
    case Kind::EasyStrides:  return "Easy+str";
    case Kind::Long:         return "Long";
    case Kind::LongFinish:   return "Long+fast";
    case Kind::Tempo:        return "Tempo";
    case Kind::Cruise:       return "Cruise";
    case Kind::Intervals:    return "Intervals";
    case Kind::ThirtyThirty: return "30-30s";
    case Kind::Hills:        return "Hills";
    case Kind::Reps:         return "Reps";
    case Kind::Pace10k:      return "10K pace";
    case Kind::PaceHalf:     return "Half pace";
    case Kind::TimeTrial5k:  return "5K trial";
    case Kind::TimeTrial10k: return "10K trial";
    case Kind::Shakeout:     return "Shake-out";
    case Kind::Race:         return "Race";
    }
    return "";
}

/// What the session is for (PLANS.md section 3.1).
inline const char* purpose(Kind k)
{
    switch (k) {
    case Kind::Rest:         return "Rest is training too";
    case Kind::Easy:         return "Easy: you should be able to talk";
    case Kind::EasyStrides:  return "Easy, with strides for form";
    case Kind::Long:         return "Builds your endurance";
    case Kind::LongFinish:   return "Endurance, finishing at race pace";
    case Kind::Tempo:        return "Raises your threshold";
    case Kind::Cruise:       return "Threshold in pieces";
    case Kind::Intervals:    return "Builds your VO2max";
    case Kind::ThirtyThirty: return "VO2max in short bites";
    case Kind::Hills:        return "Strength and form";
    case Kind::Reps:         return "Speed and form";
    case Kind::Pace10k:      return "Race pace";
    case Kind::PaceHalf:     return "Race pace";
    case Kind::TimeTrial5k:  return "Sets your paces";
    case Kind::TimeTrial10k: return "Sets your paces";
    case Kind::Shakeout:     return "Loosen up for race day";
    case Kind::Race:         return "Good luck";
    }
    return "";
}

/// "3 min", "45 s", "2:30" for a rep or recovery time.
inline int repTime(uint32_t s, char* buf, size_t cap)
{
    int n = 0;
    if (s < 60) {
        n = std::snprintf(buf, cap, "%u s", static_cast<unsigned>(s));
    } else if (s % 60 == 0) {
        n = std::snprintf(buf, cap, "%u min", static_cast<unsigned>(s / 60));
    } else {
        n = std::snprintf(buf, cap, "%u:%02u", static_cast<unsigned>(s / 60),
                          static_cast<unsigned>(s % 60));
    }
    return n < 0 ? 0 : n;
}

/// "21.1 km", "10 km", "400 m".
inline int distance(uint32_t cm, char* buf, size_t cap)
{
    const uint32_t m = (cm + 50) / 100;
    int n = 0;
    if (m < 1000) {
        n = std::snprintf(buf, cap, "%u m", static_cast<unsigned>(m));
    } else if (m % 1000 == 0) {
        n = std::snprintf(buf, cap, "%u km", static_cast<unsigned>(m / 1000));
    } else {
        const uint32_t tenths = (m + 50) / 100;
        n = std::snprintf(buf, cap, "%u.%u km", static_cast<unsigned>(tenths / 10),
                          static_cast<unsigned>(tenths % 10));
    }
    return n < 0 ? 0 : n;
}

/// The "what" line of a session as it will be run (after levels and caps):
/// "4 x 4 min", "45 min", "100 min, last 20 at half pace", "2 x 8 x 30/30".
/// @p raceCm is the plan's race distance, for Kind::Race.
inline int what(const Session& s, uint32_t raceCm, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    char t[16];
    int n = 0;
    switch (s.kind) {
    case Kind::Rest:
        break;
    case Kind::Easy:
    case Kind::EasyStrides:
    case Kind::Long:
    case Kind::Tempo:
        n = std::snprintf(buf, cap, "%u min", static_cast<unsigned>(s.a));
        break;
    case Kind::LongFinish:
        n = std::snprintf(buf, cap, "%u min, last %u at half pace", static_cast<unsigned>(s.a),
                          static_cast<unsigned>(s.b));
        break;
    case Kind::Cruise:
    case Kind::Intervals:
    case Kind::Pace10k:
    case Kind::PaceHalf:
    case Kind::Hills:
        repTime(s.a, t, sizeof(t));
        n = std::snprintf(buf, cap, "%u x %s", static_cast<unsigned>(s.reps), t);
        break;
    case Kind::ThirtyThirty:
        n = std::snprintf(buf, cap, "%u x %u x 30/30", static_cast<unsigned>(s.reps),
                          static_cast<unsigned>(s.a));
        break;
    case Kind::Reps:
        n = std::snprintf(buf, cap, "%u x %u m", static_cast<unsigned>(s.reps), static_cast<unsigned>(s.a));
        break;
    case Kind::TimeTrial5k:
        n = std::snprintf(buf, cap, "5 km");
        break;
    case Kind::TimeTrial10k:
        n = std::snprintf(buf, cap, "10 km");
        break;
    case Kind::Shakeout:
        n = std::snprintf(buf, cap, "20 min");
        break;
    case Kind::Race:
        n = distance(raceCm, buf, cap);
        break;
    }
    return n < 0 ? 0 : n;
}

}  // namespace PlanText
}  // namespace XTraining

#endif  // XTRAINING_PLAN_TEXT_HPP
