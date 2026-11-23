/**
 ******************************************************************************
 * @file    LoadFormText.hpp
 * @brief   Training form in words (LoadHistory.hpp): HybridX Streak's menu, Load
 *          screen and glance, and since Streak's S10 every X app's Saved screen.
 *          Pure C++, header-only, plain ASCII (the glance's fonts have nothing
 *          else), integer only. Each writer fills the caller's buffer, never past
 *          it, and returns its length.
 *
 * The original is here in hybridx-link (it was Streak's Core/Header/FormText.hpp
 * until 23 November 2026; renamed because X Run and X Training have a FormText.hpp
 * of their own, about running form). The namespace stays Streak's: it is Streak's
 * model, shared so the apps show the same words.
 *
 * Voice (DESIGN 7): an encouraging coach, short, saying what to do. So the
 * hardest word on the glance is "ease off", not "overreaching".
 ******************************************************************************
 */

#ifndef HYBRIDX_LINK_LOAD_FORM_TEXT_HPP
#define HYBRIDX_LINK_LOAD_FORM_TEXT_HPP

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

/// The glance's line on a banked week once form is known (NOTES "S9"): "Banked. Form +14", the
/// number held to three digits as formatFormLine's is.
inline size_t formatBankedForm(char* out, size_t cap, int16_t form)
{
    const int16_t shown = form > 999 ? 999 : (form < -999 ? -999 : form);
    char n[8];
    formatFormNumber(n, sizeof(n), shown);
    return detail::put(out, cap, std::snprintf(out, cap, "Banked. Form %s", n));
}

/// Where form stands after a session (Streak NOTES "S9", "S10"): Streak's toast after an open's
/// sessions and every X app's Saved screen. "Form now +12, fresh", or "Form +12, fresh" (or
/// "Form now +12" with no word) when the longer would pass @p maxChars.
inline size_t formatFormNow(char* out, size_t cap, int16_t form, FormWord word, size_t maxChars = 21)
{
    char n[8];
    formatFormNumber(n, sizeof(n), form);
    const char* w = formWordShort(word);
    int len = 0;
    if (w[0] != '\0') {
        len = std::snprintf(out, cap, "Form now %s, %s", n, w);
        if (len > 0 && static_cast<size_t>(len) <= maxChars) {
            return detail::put(out, cap, len);
        }
        len = std::snprintf(out, cap, "Form %s, %s", n, w);
        if (len > 0 && static_cast<size_t>(len) <= maxChars) {
            return detail::put(out, cap, len);
        }
    }
    return detail::put(out, cap, std::snprintf(out, cap, "Form now %s", n));
}

} // namespace Streak

#endif // HYBRIDX_LINK_LOAD_FORM_TEXT_HPP
