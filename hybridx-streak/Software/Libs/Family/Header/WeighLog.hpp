/**
 ******************************************************************************
 * @file    WeighLog.hpp
 * @brief   One weigh-in, and its line in the log file.
 *
 * The log is a plain CSV so it opens in a spreadsheet straight off the watch's
 * USB drive:
 *
 *     date,time,kg
 *     2026-10-12,07:31,82.30
 *
 * The date and time are local, as the watch showed them when the weigh-in was
 * made. Every weigh-in is kept; which ones count is the trend's business
 * (Trend.hpp). Pure, host-tested.
 ******************************************************************************
 */

#ifndef XWEIGHIN_WEIGH_LOG_HPP
#define XWEIGHIN_WEIGH_LOG_HPP

#include <cstddef>
#include <cstdint>

namespace XWeighIn
{

/// One weigh-in.
struct Entry {
    int32_t  day        = 0;   ///< Local day number (Calendar.hpp).
    uint16_t minute     = 0;   ///< Local minute of the day, 0-1439.
    int32_t  hundredths = 0;   ///< Weight in 0.01 kg, 1..kMaxKg * 100.
};

/// Receives entries one at a time, in log order: a scan of the file feeds
/// one, and so does a new weigh-in. Trend is one.
class EntryVisitor
{
public:
    virtual ~EntryVisitor() = default;
    virtual void onEntry(const Entry& e) = 0;
};

/// The first line of every log file.
constexpr const char* kLogHeader = "date,time,kg";

/// The longest line the reader accepts, terminator included. A valid line is
/// 22 bytes ("2026-10-12,07:31,82.30"); anything far longer is junk.
constexpr size_t kMaxLineBytes = 64;

/// True if every field is in range: a valid date in the accepted years, a
/// minute below 1440 and a weight the app accepts.
bool isValidEntry(const Entry& e);

/**
 * @brief   The entry's line, without the newline: "2026-10-12,07:31,82.30".
 * @return  Its length, or 0 if the entry is invalid or does not fit (the
 *          output is then an empty string, if outSize > 0).
 */
size_t formatEntry(const Entry& e, char* out, size_t outSize);

/**
 * @brief   Parse one line, without its newline (a trailing '\r' is allowed).
 *
 * Strict: YYYY-MM-DD,HH:MM,kg with 0, 1 or 2 decimals and nothing else. The
 * header, a blank line, a torn line and junk are all simply "not an entry".
 *
 * @return  true and @p out filled for a valid entry; false otherwise, with
 *          @p out untouched.
 */
bool parseLine(const char* line, size_t len, Entry& out);

}  // namespace XWeighIn

#endif  // XWEIGHIN_WEIGH_LOG_HPP
