/**
 ******************************************************************************
 * @file    WeightUnit.hpp
 * @brief   The unit weights are shown and entered in. Display only: the log,
 *          the trend and every message hold weights in 0.01 kg whatever the
 *          unit, so changing it never touches the data.
 *
 * Header-only, integers only (no %f on the watch), host-tested
 * (WeightUnitTest). Config field `units`: 0 kg, 1 stones and pounds, 2 pounds.
 ******************************************************************************
 */

#ifndef XWEIGHIN_WEIGHT_UNIT_HPP
#define XWEIGHIN_WEIGHT_UNIT_HPP

#include <cstdint>

namespace XWeighIn
{

enum class WeightUnit : uint8_t {
    Kg   = 0,   ///< Metric: kilograms.
    StLb = 1,   ///< Imperial: stones and pounds, 14 lb to the stone.
    Lb   = 2,   ///< Imperial: pounds alone.
};

constexpr uint8_t kWeightUnitCount = 3;

/// The config field's value as a unit; anything out of range is kg.
inline WeightUnit weightUnitFromConfig(int32_t value)
{
    return (value >= 0 && value < kWeightUnitCount) ? static_cast<WeightUnit>(value) : WeightUnit::Kg;
}

constexpr int32_t kLbPerStone = 14;

/// 0.01 kg to tenths of a lb, rounded to nearest (1 kg = 2.20462262 lb).
/// 0 or less gives 0. int64 inside, so the largest weight cannot overflow.
inline int32_t hundredthsToLbTenths(int32_t hundredths)
{
    if (hundredths <= 0) {
        return 0;
    }
    const int64_t scaled = static_cast<int64_t>(hundredths) * 220462262 + 500000000;
    return static_cast<int32_t>(scaled / 1000000000);
}

/// Tenths of a lb to 0.01 kg, rounded to nearest (1 lb = 0.45359237 kg).
inline int32_t lbTenthsToHundredths(int32_t tenths)
{
    if (tenths <= 0) {
        return 0;
    }
    const int64_t scaled = static_cast<int64_t>(tenths) * 45359237 + 5000000;
    return static_cast<int32_t>(scaled / 10000000);
}

/// A signed change in 0.01 kg to tenths of a lb, rounding half away from zero.
inline int32_t changeToLbTenths(int32_t hundredths)
{
    return hundredths < 0 ? -hundredthsToLbTenths(-hundredths) : hundredthsToLbTenths(hundredths);
}

/// The unit's name for a weight: "kg", "st lb" or "lb".
inline const char* weightUnitName(WeightUnit unit)
{
    switch (unit) {
        case WeightUnit::StLb: return "st lb";
        case WeightUnit::Lb:   return "lb";
        default:               return "kg";
    }
}

/// The unit a change is in: stones are too big for a week's change, so
/// stones and pounds show it in pounds.
inline const char* changeUnitName(WeightUnit unit)
{
    return unit == WeightUnit::Kg ? "kg" : "lb";
}

}  // namespace XWeighIn

#endif  // XWEIGHIN_WEIGHT_UNIT_HPP
