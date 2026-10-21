/**
 ******************************************************************************
 * @file    FixedPoint.hpp
 * @brief   Float readings as hundredths, and hundredths as text. Pure C++,
 *          header-only (the GUI uses it).
 *
 * The embedded rules forbid %f, so a reading travels and is logged as an
 * integer count of hundredths ("x100"): 23.45 is 2345. kX100Invalid stands for
 * "no reading". The formatter writes into a caller's buffer, never past it,
 * and always terminates it.
 ******************************************************************************
 */

#ifndef SAUNA_FIXED_POINT_HPP
#define SAUNA_FIXED_POINT_HPP

#include <cstddef>
#include <cstdint>

namespace Sauna {

/// "No reading yet", or one that was not a number.
constexpr int32_t kX100Invalid = INT32_MIN;

/// Readings beyond this saturate: 20 million units is 2e9 hundredths, which
/// still fits an int32_t, and no sensor here comes close.
constexpr float kX100Limit = 20000000.0f;

/// A float in hundredths, rounded half away from zero. NaN is kX100Invalid;
/// infinities and anything beyond kX100Limit saturate.
inline int32_t toX100(float v)
{
    if (!(v == v)) {
        return kX100Invalid;
    }
    if (v >= kX100Limit) {
        return static_cast<int32_t>(kX100Limit * 100.0f);
    }
    if (v <= -kX100Limit) {
        return -static_cast<int32_t>(kX100Limit * 100.0f);
    }
    return static_cast<int32_t>(v * 100.0f + (v < 0.0f ? -0.5f : 0.5f));
}

/**
 * @brief   Write hundredths as decimal text: "23.4", "-0.50", "7".
 * @param   decimals  0, 1 or 2 digits after the point (larger values act as 2);
 *                    the value is rounded half away from zero to that many.
 * @return  Characters written, not counting the terminator. kX100Invalid gives
 *          "--". Text that does not fit is cut short; with cap == 0 nothing is
 *          written.
 */
inline size_t formatX100(char *out, size_t cap, int32_t x100, uint8_t decimals = 2)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }

    char   tmp[24];
    size_t n = 0;

    if (x100 == kX100Invalid) {
        tmp[n++] = '-';
        tmp[n++] = '-';
    } else {
        if (decimals > 2) {
            decimals = 2;
        }
        const uint32_t scale = decimals == 2 ? 1u : (decimals == 1 ? 10u : 100u);
        // Magnitude through unsigned arithmetic, so INT32_MIN + 1 is safe.
        const uint32_t mag = x100 < 0 ? 0u - static_cast<uint32_t>(x100) : static_cast<uint32_t>(x100);
        const uint32_t m   = (mag + scale / 2u) / scale;

        uint32_t pow10 = 1;
        for (uint8_t i = 0; i < decimals; ++i) {
            pow10 *= 10u;
        }
        const uint32_t whole = m / pow10;
        uint32_t       frac  = m % pow10;

        if (x100 < 0 && m != 0) {
            tmp[n++] = '-';
        }

        char digits[12];
        int  d = 0;
        uint32_t w = whole;
        do {
            digits[d++] = static_cast<char>('0' + (w % 10u));
            w /= 10u;
        } while (w != 0 && d < static_cast<int>(sizeof(digits)));
        while (d > 0) {
            tmp[n++] = digits[--d];
        }

        if (decimals > 0) {
            tmp[n++] = '.';
            char f[2] = {'0', '0'};
            for (int i = decimals - 1; i >= 0; --i) {
                f[i] = static_cast<char>('0' + (frac % 10u));
                frac /= 10u;
            }
            for (uint8_t i = 0; i < decimals; ++i) {
                tmp[n++] = f[i];
            }
        }
    }

    const size_t len = n < cap - 1 ? n : cap - 1;
    for (size_t i = 0; i < len; ++i) {
        out[i] = tmp[i];
    }
    out[len] = '\0';
    return len;
}

} // namespace Sauna

#endif // SAUNA_FIXED_POINT_HPP
