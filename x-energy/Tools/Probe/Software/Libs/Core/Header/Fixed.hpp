/**
 ******************************************************************************
 * @file    Fixed.hpp
 * @brief   Floats as scaled integers and as fixed-point text, with no %f
 *          (the standing embedded rules). Header-only.
 ******************************************************************************
 */

#ifndef ENERGY_PROBE_FIXED_HPP
#define ENERGY_PROBE_FIXED_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace EnergyProbe
{

/// Out of range or not a number.
constexpr int64_t kNoValue = INT64_MIN;

/// v * scale, rounded half away from zero; kNoValue for NaN, infinity or beyond +-9e15.
inline int64_t scaled(float v, int32_t scale)
{
    if (!(v == v)) {   // NaN
        return kNoValue;
    }
    const double d = static_cast<double>(v) * scale;
    if (d > 9.0e15 || d < -9.0e15) {
        return kNoValue;
    }
    return static_cast<int64_t>(d >= 0 ? d + 0.5 : d - 0.5);
}

/// v with `decimals` places (0-6), e.g. "-12.345"; "nan" when it has no value.
/// Returns the length written (snprintf's rule: truncated if cap is too small).
inline int formatFixed(char* out, size_t cap, float v, uint8_t decimals)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    if (decimals > 6) {
        decimals = 6;
    }
    int32_t scale = 1;
    for (uint8_t i = 0; i < decimals; ++i) {
        scale *= 10;
    }
    const int64_t s = scaled(v, scale);
    if (s == kNoValue) {
        return std::snprintf(out, cap, "nan");
    }
    // Printed as two 32-bit halves: the watch's printf may not do 64-bit integers.
    const bool     neg   = s < 0;
    const uint64_t mag   = neg ? static_cast<uint64_t>(-s) : static_cast<uint64_t>(s);
    const uint64_t whole = mag / static_cast<uint64_t>(scale);
    const uint64_t frac  = mag % static_cast<uint64_t>(scale);
    if (whole > 0xFFFFFFFFull) {
        return std::snprintf(out, cap, "nan");
    }
    if (decimals == 0) {
        return std::snprintf(out, cap, "%s%lu", neg ? "-" : "", static_cast<unsigned long>(whole));
    }
    return std::snprintf(out, cap, "%s%lu.%0*lu", neg ? "-" : "", static_cast<unsigned long>(whole),
                         static_cast<int>(decimals), static_cast<unsigned long>(frac));
}

} // namespace EnergyProbe

#endif // ENERGY_PROBE_FIXED_HPP
