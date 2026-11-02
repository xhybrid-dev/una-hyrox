/**
 ******************************************************************************
 * @file    FormText.hpp
 * @brief   Training form in words, for the menu, the Load screen and the
 *          glance (LoadHistory.hpp). Pure C++, header-only, plain ASCII (the
 *          glance's fonts have nothing else), integer only. Each writer fills
 *          the caller's buffer, never past it, and returns its length.
 *
 * Voice (DESIGN 7): an encouraging coach, short, saying what to do. So the
 * hardest word on the glance is "ease off", not "overreaching".
 ******************************************************************************
 */

#ifndef STREAK_FORM_TEXT_HPP
#define STREAK_FORM_TEXT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "LoadHistory.hpp"

namespace Streak
{

/// The word on the Load screen.
inline const char* formWordName(FormWord w)
{
    switch (w) {
        case FormWord::NoLoad:       return "No load yet";
        case FormWord::Fresh:        return "Fresh";
        case FormWord::Steady:       return "Steady";
        case FormWord::Building:     return "Building";
        case FormWord::Overreaching: return "Overreaching";
        case FormWord::Learning:
        default:                     return "Learning";
    }
}

/// The word in a line (lower case, and the coach's advice for the hardest).
inline const char* formWordShort(FormWord w)
{
    switch (w) {
        case FormWord::Fresh:        return "fresh";
        case FormWord::Steady:       return "steady";
        case FormWord::Building:     return "building";
        case FormWord::Overreaching: return "ease off";
        default:                     return "";
    }
}

namespace detail
{
inline size_t put(char* out, size_t cap, int n)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    if (n < 0) {
        out[0] = '\0';
        return 0;
    }
    return static_cast<size_t>(n) < cap ? static_cast<size_t>(n) : cap - 1;
}
} // namespace detail

/// Form as the Load screen's number: "+5", "-12", "0".
inline size_t formatFormNumber(char* out, size_t cap, int16_t form)
{
    return detail::put(out, cap, form > 0 ? std::snprintf(out, cap, "+%d", static_cast<int>(form))
                                          : std::snprintf(out, cap, "%d", static_cast<int>(form)));
}

/// The one line: "Form +5 fresh", "Form -30 ease off", "Learning: day 5 of 14",
/// "No load yet". For the menu's hint and the glance's bottom line.
inline size_t formatFormLine(char* out, size_t cap, int16_t form, FormWord word, uint16_t historyDays)
{
    if (word == FormWord::Learning) {
        const unsigned day = historyDays < 1 ? 1u : (historyDays > kWarmUpDays ? kWarmUpDays : historyDays);
        return detail::put(out, cap, std::snprintf(out, cap, "Learning: day %u of %u", day,
                                                   static_cast<unsigned>(kWarmUpDays)));
    }
    if (word == FormWord::NoLoad) {
        return detail::put(out, cap, std::snprintf(out, cap, "%s", "No load yet"));
    }
    // The line is the glance's, 176 px at most: the number is held to three digits
    // (the Load screen shows it in full), and no comma.
    const int16_t shown = form > 999 ? 999 : (form < -999 ? -999 : form);
    char n[8];
    formatFormNumber(n, sizeof(n), shown);
    return detail::put(out, cap, std::snprintf(out, cap, "Form %s %s", n, formWordShort(word)));
}

} // namespace Streak

#endif // STREAK_FORM_TEXT_HPP
