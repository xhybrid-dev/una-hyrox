/**
 ******************************************************************************
 * @file    Vo2Text.cpp
 * @brief   VO2max text for the screens.
 ******************************************************************************
 */

#include "Vo2Text.hpp"

#include <cstdio>

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
        return "Not enough steady running";
    case RunStatus::ProfileIncomplete:
        switch (r.profileStatus) {
        case ProfileStatus::NeedsAge:        return "Set birth year or max HR";
        case ProfileStatus::NeedsRestingHr:  return "Set resting HR";
        case ProfileStatus::ReserveTooSmall: return "Check max and resting HR";
        case ProfileStatus::Ok:              break;
        }
        break;
    }
    return "";
}

}  // namespace RunVo2::Text
