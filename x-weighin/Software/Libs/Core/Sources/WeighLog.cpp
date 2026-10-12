/**
 ******************************************************************************
 * @file    WeighLog.cpp
 * @brief   One weigh-in's line in the log file (see the header).
 ******************************************************************************
 */

#include "WeighLog.hpp"

#include "Calendar.hpp"
#include "WeighProfile.hpp"

#include <cstdio>

namespace XWeighIn
{

namespace
{

constexpr int32_t kMaxHundredths = kMaxKg * 100;

/// Read exactly @p n digits at @p p into @p value.
bool digits(const char* p, size_t n, int32_t& value)
{
    int32_t v = 0;
    for (size_t i = 0; i < n; ++i) {
        if (p[i] < '0' || p[i] > '9') {
            return false;
        }
        v = v * 10 + (p[i] - '0');
    }
    value = v;
    return true;
}

}  // namespace

bool isValidEntry(const Entry& e)
{
    return e.day >= firstValidDay() && e.day <= lastValidDay() &&
           e.minute < 1440 &&
           e.hundredths > 0 && e.hundredths <= kMaxHundredths;
}

size_t formatEntry(const Entry& e, char* out, size_t outSize)
{
    if (out == nullptr || outSize == 0) {
        return 0;
    }
    out[0] = '\0';
    if (!isValidEntry(e)) {
        return 0;
    }
    const Date d = civilFromDays(e.day);
    const int  n = std::snprintf(out, outSize, "%04d-%02u-%02u,%02u:%02u,%d.%02d",
                                 static_cast<int>(d.year),
                                 static_cast<unsigned>(d.month), static_cast<unsigned>(d.day),
                                 static_cast<unsigned>(e.minute / 60), static_cast<unsigned>(e.minute % 60),
                                 static_cast<int>(e.hundredths / 100), static_cast<int>(e.hundredths % 100));
    if (n <= 0 || static_cast<size_t>(n) >= outSize) {
        out[0] = '\0';
        return 0;
    }
    return static_cast<size_t>(n);
}

bool parseLine(const char* line, size_t len, Entry& out)
{
    if (line == nullptr) {
        return false;
    }
    if (len > 0 && line[len - 1] == '\r') {
        --len;
    }
    // "YYYY-MM-DD,HH:MM," is 17 bytes; the weight needs at least one digit.
    if (len < 18 || len >= kMaxLineBytes) {
        return false;
    }

    int32_t y = 0, mo = 0, d = 0, h = 0, mi = 0;
    if (!digits(line, 4, y) || line[4] != '-' ||
        !digits(line + 5, 2, mo) || line[7] != '-' ||
        !digits(line + 8, 2, d) || line[10] != ',' ||
        !digits(line + 11, 2, h) || line[13] != ':' ||
        !digits(line + 14, 2, mi) || line[16] != ',') {
        return false;
    }
    if (!isValidDate(y, static_cast<uint32_t>(mo), static_cast<uint32_t>(d)) || h > 23 || mi > 59) {
        return false;
    }

    // The weight: 1-3 whole digits, then optionally '.' and 1-2 decimals.
    const char* p   = line + 17;
    const char* end = line + len;
    int32_t whole = 0;
    int     wholeDigits = 0;
    while (p < end && *p >= '0' && *p <= '9') {
        if (++wholeDigits > 3) {
            return false;
        }
        whole = whole * 10 + (*p - '0');
        ++p;
    }
    if (wholeDigits == 0) {
        return false;
    }
    int32_t fraction = 0;
    if (p < end && *p == '.') {
        ++p;
        int decimals = 0;
        while (p < end && *p >= '0' && *p <= '9') {
            if (++decimals > 2) {
                return false;
            }
            fraction = fraction * 10 + (*p - '0');
            ++p;
        }
        if (decimals == 0) {
            return false;
        }
        if (decimals == 1) {
            fraction *= 10;
        }
    }
    if (p != end) {
        return false;   // anything else on the line: an extra field, junk
    }

    Entry e;
    e.day        = daysFromCivil(y, static_cast<uint32_t>(mo), static_cast<uint32_t>(d));
    e.minute     = static_cast<uint16_t>(h * 60 + mi);
    e.hundredths = whole * 100 + fraction;
    if (!isValidEntry(e)) {
        return false;
    }
    out = e;
    return true;
}

}  // namespace XWeighIn
