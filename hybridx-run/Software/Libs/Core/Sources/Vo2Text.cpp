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

const char* reason(const RunResult& r)
{
    switch (r.status) {
    case RunStatus::Ok:
        return "";
    case RunStatus::NotEnoughRunning:
        return "Need 5 steady minutes";
    case RunStatus::ProfileIncomplete:
        switch (r.profileStatus) {
        case ProfileStatus::NeedsAge:        return "Set your birth year";
        case ProfileStatus::NeedsRestingHr:  return "Set resting HR";
        case ProfileStatus::ReserveTooSmall: return "Check HR settings";
        case ProfileStatus::Ok:              break;
        }
        break;
    }
    return "";
}

}  // namespace RunVo2::Text
