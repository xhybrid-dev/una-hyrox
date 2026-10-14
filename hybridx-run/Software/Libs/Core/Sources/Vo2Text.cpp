/**
 ******************************************************************************
 * @file    Vo2Text.cpp
 * @brief   VO2max text for the screens.
 ******************************************************************************
 */

#include "Vo2Text.hpp"

#include <cstdio>

// The reasons fit one line of the summary face (about 20 characters in
// Regular 16 on the round screen).
static_assert(RunVo2::Config::kMinWindows == 5, "the reason below names 5 minutes");

namespace RunVo2::Text
{

const char* formatX10(uint16_t x10, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return "";
    }
    if (x10 == 0) {
        std::snprintf(buf, cap, "---");
    } else {
        std::snprintf(buf, cap, "%u.%u", static_cast<unsigned>(x10 / 10),
                      static_cast<unsigned>(x10 % 10));
    }
    return buf;
}

const char* profileReason(ProfileStatus s)
{
    switch (s) {
    case ProfileStatus::NeedsAge:        return "Set your birth year";
    case ProfileStatus::NeedsRestingHr:  return "Set resting HR";
    case ProfileStatus::ReserveTooSmall: return "Check HR settings";
    case ProfileStatus::Ok:              break;
    }
    return "";
}

const char* shortDate(uint8_t day, uint8_t month, char* buf, size_t cap)
{
    static const char* const kMonths[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (buf == nullptr || cap == 0) {
        return "";
    }
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        buf[0] = '\0';
        return buf;
    }
    std::snprintf(buf, cap, "%u %s", static_cast<unsigned>(day), kMonths[month - 1]);
    return buf;
}

const char* trend(uint16_t nowX10, uint16_t beforeX10, char* buf, size_t cap)
{
    if (buf == nullptr || cap == 0) {
        return "";
    }
    if (nowX10 == 0 || beforeX10 == 0) {
        buf[0] = '\0';
    } else if (nowX10 == beforeX10) {
        std::snprintf(buf, cap, "Same as last run");
    } else {
        const bool     up = nowX10 > beforeX10;
        const unsigned d  = up ? nowX10 - beforeX10 : beforeX10 - nowX10;
        std::snprintf(buf, cap, "%c%u.%u since last run", up ? '+' : '-', d / 10, d % 10);
    }
    return buf;
}

const char* maxSourceLabel(MaxHrSource s)
{
    switch (s) {
    case MaxHrSource::Entered:  return "entered";
    case MaxHrSource::Formula:  return "age";
    case MaxHrSource::Observed: return "runs";
    case MaxHrSource::None:     break;
    }
    return "";
}

const char* restSourceLabel(RestHrSource s)
{
    switch (s) {
    case RestHrSource::Entered: return "entered";
    case RestHrSource::Watch:   return "watch";
    case RestHrSource::None:    break;
    }
    return "";
}

const char* reason(const RunResult& r)
{
    switch (r.status) {
    case RunStatus::Ok:
        return "";
    case RunStatus::NotEnoughRunning:
        return "Need 5 steady minutes";
    case RunStatus::TooEasy:
        return "Too easy to estimate";
    case RunStatus::HrAboveMax:
        return "HR above your max";
    case RunStatus::ProfileIncomplete:
        return profileReason(r.profileStatus);
    }
    return "";
}

}  // namespace RunVo2::Text
