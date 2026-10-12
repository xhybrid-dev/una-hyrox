/**
 ******************************************************************************
 * @file    BodyMetrics.cpp
 * @brief   BMI and resting energy (see the header).
 ******************************************************************************
 */

#include "BodyMetrics.hpp"

#include "WeighProfile.hpp"
#include "WeighText.hpp"

namespace XWeighIn
{

namespace
{

bool weightSet(int32_t hundredths)
{
    return hundredths > 0 && hundredths <= kMaxKg * 100;
}

bool heightSet(uint32_t heightCm)
{
    return heightCm >= kMinHeightCm && heightCm <= kMaxHeightCm;
}

}  // namespace

int32_t bmiTenths(int32_t hundredths, uint32_t heightCm)
{
    if (!weightSet(hundredths) || !heightSet(heightCm)) {
        return 0;
    }
    // kg / m^2 = (h / 100) / (cm / 100)^2 = 100 h / cm^2; in tenths, 1000 h / cm^2.
    const int64_t num = static_cast<int64_t>(hundredths) * 1000;
    const int64_t den = static_cast<int64_t>(heightCm) * heightCm;
    return static_cast<int32_t>((num + den / 2) / den);
}

int32_t bmrKcal(int32_t hundredths, uint32_t heightCm, uint8_t ageYears, uint8_t sex)
{
    if (!weightSet(hundredths) || !heightSet(heightCm) || ageYears == 0) {
        return 0;
    }
    int32_t s = 0;
    switch (static_cast<Sex>(sex)) {
        case Sex::Male:   s = 5;    break;
        case Sex::Female: s = -161; break;
        default:          return 0;
    }
    // In hundredths of a kcal: 10 x kg is 10 h / 100 kcal = 10 h hundredths;
    // 6.25 x cm is 625 cm hundredths; 5 x age is 500 age; s is 100 s.
    const int64_t c = 10LL * hundredths + 625LL * heightCm - 500LL * ageYears + 100LL * s;
    if (c <= 0) {
        return 0;
    }
    return static_cast<int32_t>((c + 50) / 100);
}

}  // namespace XWeighIn
