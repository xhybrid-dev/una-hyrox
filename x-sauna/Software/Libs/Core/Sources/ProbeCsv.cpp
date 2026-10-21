/**
 ******************************************************************************
 * @file    ProbeCsv.cpp
 * @brief   The probe's CSV lines (see ProbeCsv.hpp).
 ******************************************************************************
 */

#include "ProbeCsv.hpp"

#include <cstdio>

namespace Sauna {

namespace {

/// Append @p text to @p out at @p pos; false (and nothing more) if it does not fit.
bool put(char *out, size_t cap, size_t &pos, const char *text)
{
    while (*text != '\0') {
        if (pos + 1 >= cap) {
            return false;
        }
        out[pos++] = *text++;
    }
    return true;
}

/// A signed field in hundredths of nothing: an integer. Blank when invalid.
bool putSigned(char *out, size_t cap, size_t &pos, int32_t v, bool blank)
{
    if (blank) {
        return true;
    }
    char tmp[16];
    std::snprintf(tmp, sizeof(tmp), "%ld", static_cast<long>(v));
    return put(out, cap, pos, tmp);
}

bool putUnsigned(char *out, size_t cap, size_t &pos, uint32_t v)
{
    char tmp[16];
    std::snprintf(tmp, sizeof(tmp), "%lu", static_cast<unsigned long>(v));
    return put(out, cap, pos, tmp);
}

constexpr char kComma[] = ",";

} // namespace

size_t formatCsvHeader(char *out, size_t cap)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    size_t pos = 0;
    if (!put(out, cap, pos,
             "ms,t_s,mark,temp_x100,temp_n,temp_bad,temp_fields,temp_gap_ms,hr,trust_x100,hr_n\n")) {
        out[0] = '\0';
        return 0;
    }
    out[pos] = '\0';
    return pos;
}

size_t formatCsvRow(char *out, size_t cap, const CsvRow &row)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    size_t pos = 0;
    bool   ok  = true;

    ok = ok && putUnsigned(out, cap, pos, row.tMs) && put(out, cap, pos, kComma);
    ok = ok && putUnsigned(out, cap, pos, row.tS) && put(out, cap, pos, kComma);
    ok = ok && putUnsigned(out, cap, pos, row.mark) && put(out, cap, pos, kComma);
    ok = ok && putSigned(out, cap, pos, row.tempX100, row.tempX100 == kX100Invalid) && put(out, cap, pos, kComma);
    ok = ok && putUnsigned(out, cap, pos, row.tempN) && put(out, cap, pos, kComma);
    ok = ok && putUnsigned(out, cap, pos, row.tempBad) && put(out, cap, pos, kComma);
    ok = ok && putUnsigned(out, cap, pos, row.tempFields) && put(out, cap, pos, kComma);
    ok = ok && putUnsigned(out, cap, pos, row.tempGapMs) && put(out, cap, pos, kComma);
    // Heart rate is blank until a sample has arrived, not a misleading 0.
    if (row.hrN != 0) {
        ok = ok && putUnsigned(out, cap, pos, row.hr);
    }
    ok = ok && put(out, cap, pos, kComma);
    ok = ok && putSigned(out, cap, pos, row.trustX100, row.hrN == 0 || row.trustX100 == kX100Invalid) &&
         put(out, cap, pos, kComma);
    ok = ok && putUnsigned(out, cap, pos, row.hrN) && put(out, cap, pos, "\n");

    if (!ok) {
        out[0] = '\0';
        return 0;
    }
    out[pos] = '\0';
    return pos;
}

size_t formatRunLine(char *out, size_t cap, uint16_t run, const char *stamp)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    size_t pos = 0;
    bool   ok  = put(out, cap, pos, "# run ") && putUnsigned(out, cap, pos, run) && put(out, cap, pos, " ") &&
               put(out, cap, pos, stamp != nullptr ? stamp : "") && put(out, cap, pos, "\n");
    if (!ok) {
        out[0] = '\0';
        return 0;
    }
    out[pos] = '\0';
    return pos;
}

size_t formatNoteLine(char *out, size_t cap, const char *text)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    size_t pos = 0;
    bool   ok  = put(out, cap, pos, "# ");
    for (const char *p = text != nullptr ? text : ""; ok && *p != '\0'; ++p) {
        const char c[2] = {(*p == '\n' || *p == '\r') ? ' ' : *p, '\0'};
        ok = put(out, cap, pos, c);
    }
    ok = ok && put(out, cap, pos, "\n");
    if (!ok) {
        out[0] = '\0';
        return 0;
    }
    out[pos] = '\0';
    return pos;
}

void RunCounter::feed(const char *data, size_t len)
{
    const uint8_t tagLen = static_cast<uint8_t>(sizeof(kTag) - 1);
    for (size_t i = 0; i < len; ++i) {
        const char c = data[i];
        if (c == '\n') {
            mAtLineStart = true;
            mMatched     = 0;
            mMismatch    = false;
            continue;
        }
        if (!mAtLineStart && mMatched == 0 && !mMismatch) {
            // Past the tag: nothing more to learn on this line.
            continue;
        }
        mAtLineStart = false;
        if (mMismatch) {
            continue;
        }
        if (c == kTag[mMatched]) {
            if (++mMatched == tagLen) {
                if (mRuns < UINT16_MAX) {
                    ++mRuns;
                }
                mMatched = 0;      // done with this line
            }
        } else {
            mMismatch = true;
        }
    }
}

} // namespace Sauna
