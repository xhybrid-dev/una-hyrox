/**
 ******************************************************************************
 * @file    WeighProfile.cpp
 * @brief   Age and the profile's weight as integers.
 ******************************************************************************
 */

#include "WeighProfile.hpp"

#include "Calendar.hpp"

namespace XWeighIn
{

bool clockIsSet(std::time_t nowUtc)
{
    return static_cast<int64_t>(nowUtc) >= kClockFloorUtc;
}

uint8_t ageAt(uint16_t birthYear, uint8_t birthMonth, std::time_t nowUtc)
{
    if (birthYear == 0 || !clockIsSet(nowUtc)) {
        return 0;
    }
    const uint8_t month = (birthMonth >= 1 && birthMonth <= 12) ? birthMonth : 1;

    const Date today = civilFromDays(static_cast<int32_t>(static_cast<int64_t>(nowUtc) / 86400));

    int32_t age = today.year - static_cast<int32_t>(birthYear);
    if (today.month < month) {
        --age;
    }
    if (age <= 0 || age > 120) {
        return 0;
    }
    return static_cast<uint8_t>(age);
}

int32_t kgToHundredths(float kg)
{
    // NaN fails both comparisons, so it lands here too.
    if (!(kg > 0.0f && kg <= static_cast<float>(kMaxKg))) {
        return 0;
    }
    // In range, so the product fits easily; + 0.5 rounds a positive value to
    // nearest. Truncating would be wrong: 80.7f is 80.6999969..., which times
    // 100 is 8069.9995 and must read 80.70, not 80.69.
    const int32_t h = static_cast<int32_t>(kg * 100.0f + 0.5f);
    return h > 0 ? h : 0;
}

}  // namespace XWeighIn
