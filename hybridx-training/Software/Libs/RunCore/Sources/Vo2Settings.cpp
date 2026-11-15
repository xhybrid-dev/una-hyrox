/**
 ******************************************************************************
 * @file    Vo2Settings.cpp
 * @brief   Picker ranges and steps for the athlete's numbers.
 ******************************************************************************
 */

#include "Vo2Settings.hpp"

namespace RunVo2::Settings
{

namespace
{

/// The shared shape of max and resting HR: 0, then [lo, hi].
uint8_t stepWithOff(uint8_t value, int dir, uint8_t inUse, uint8_t lo, uint8_t hi)
{
    if (value == 0 || value < lo || value > hi) {
        if (dir < 0) {
            return 0;
        }
        return inUse < lo ? lo : (inUse > hi ? hi : inUse);
    }
    if (dir < 0) {
        return value == lo ? 0 : static_cast<uint8_t>(value - 1);
    }
    return value == hi ? hi : static_cast<uint8_t>(value + 1);
}

}  // namespace

void seedBirth(uint16_t& year, uint8_t& month)
{
    if (year < kMinBirthYear || year > kMaxBirthYear) {
        year  = kDefaultBirthYear;
        month = 1;
    }
    if (month < 1 || month > 12) {
        month = 1;
    }
}

uint8_t stepMonth(uint8_t month, int dir)
{
    if (month < 1 || month > 12) {
        return 1;
    }
    if (dir < 0) {
        return month == 1 ? 1 : static_cast<uint8_t>(month - 1);
    }
    return month == 12 ? 12 : static_cast<uint8_t>(month + 1);
}

uint16_t stepYear(uint16_t year, int dir)
{
    if (year < kMinBirthYear || year > kMaxBirthYear) {
        return kDefaultBirthYear;
    }
    if (dir < 0) {
        return year == kMinBirthYear ? year : static_cast<uint16_t>(year - 1);
    }
    return year == kMaxBirthYear ? year : static_cast<uint16_t>(year + 1);
}

uint8_t stepMaxHr(uint8_t value, int dir, uint8_t inUse)
{
    return stepWithOff(value, dir, inUse, kMinMaxHr, kMaxMaxHr);
}

uint8_t stepRestHr(uint8_t value, int dir, uint8_t inUse)
{
    return stepWithOff(value, dir, inUse, kMinRestHr, kMaxRestHr);
}

}  // namespace RunVo2::Settings
