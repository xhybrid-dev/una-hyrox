/**
 ******************************************************************************
 * @file    WeighProfile.cpp
 * @brief   Age and the profile's weight as integers.
 ******************************************************************************
 */

#include "WeighProfile.hpp"

namespace XWeighIn
{

namespace
{

// Civil date from days since 1970-01-01 (H. Hinnant's days_from_civil inverse).
void civilFromDays(int64_t z, int32_t& y, uint32_t& m)
{
    z += 719468;
    const int64_t  era = (z >= 0 ? z : z - 146096) / 146097;
    const uint32_t doe = static_cast<uint32_t>(z - era * 146097);
    const uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const uint32_t mp  = (5 * doy + 2) / 153;
    m = mp < 10 ? mp + 3 : mp - 9;
    y = static_cast<int32_t>(yoe + era * 400 + (m <= 2 ? 1 : 0));
}

}  // namespace

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

    int32_t  y = 0;
    uint32_t m = 0;
    civilFromDays(static_cast<int64_t>(nowUtc) / 86400, y, m);

    int32_t age = y - static_cast<int32_t>(birthYear);
    if (m < month) {
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
