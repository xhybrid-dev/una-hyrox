/**
 ******************************************************************************
 * @file    WeighText.hpp
 * @brief   Text for the screens, from integers only (no %f on the watch).
 *
 * Header-only, because the GUI uses it and the GUI ELF links no Libs sources.
 * Host-tested in Tests/Host.
 ******************************************************************************
 */

#ifndef XWEIGHIN_WEIGH_TEXT_HPP
#define XWEIGHIN_WEIGH_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace XWeighIn
{

/// Config field `sex`: 0 not set, 1 female, 2 male.
enum class Sex : uint8_t { NotSet = 0, Female = 1, Male = 2 };

/// "female", "male", or nullptr when not set or out of range.
inline const char* sexLabel(uint8_t sex)
{
    switch (static_cast<Sex>(sex)) {
        case Sex::Female: return "female";
        case Sex::Male:   return "male";
        default:          return nullptr;
    }
}

/**
 * @brief   A weight in hundredths of a kg as "82.30" (2 decimals), "82.3" (1)
 *          or "82" (0), rounding half up. No unit.
 *
 * @param   hundredths  The weight; 0 or less, or above kMaxFormatHundredths,
 *                      is "not set" and gives "--".
 * @param   decimals    0, 1 or 2; anything larger is taken as 2.
 * @param   out         Destination; always NUL-terminated if outSize > 0.
 * @return  The length of the full text. Truncated if that is >= outSize, as
 *          snprintf does; 0 if outSize is 0.
 */
inline size_t formatKg(int32_t hundredths, uint8_t decimals, char* out, size_t outSize)
{
    // Far above any weight the app accepts, and low enough that the rounding
    // below cannot overflow an int.
    constexpr int32_t kMaxFormatHundredths = 9999999;

    if (out == nullptr || outSize == 0) {
        return 0;
    }
    int n = 0;
    if (hundredths <= 0 || hundredths > kMaxFormatHundredths) {
        n = std::snprintf(out, outSize, "--");
    } else if (decimals == 0) {
        const int whole = (static_cast<int>(hundredths) + 50) / 100;
        n = std::snprintf(out, outSize, "%d", whole);
    } else if (decimals == 1) {
        const int tenths = (static_cast<int>(hundredths) + 5) / 10;
        n = std::snprintf(out, outSize, "%d.%d", tenths / 10, tenths % 10);
    } else {
        const int h = static_cast<int>(hundredths);
        n = std::snprintf(out, outSize, "%d.%02d", h / 100, h % 100);
    }
    return n > 0 ? static_cast<size_t>(n) : 0;
}

}  // namespace XWeighIn

#endif  // XWEIGHIN_WEIGH_TEXT_HPP
