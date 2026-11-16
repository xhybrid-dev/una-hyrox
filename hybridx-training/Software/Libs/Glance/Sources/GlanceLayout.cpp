/**
 ******************************************************************************
 * @file    GlanceLayout.cpp
 * @brief   What X Training's glance draws (see the header).
 ******************************************************************************
 */

#include "GlanceLayout.hpp"

#include <cstdio>
#include <cstring>

#include "GlanceFontWidths.hpp"
#include "PlanText.hpp"
#include "TimeText.hpp"

namespace TrainingGlance
{

using namespace XTraining;

namespace
{
// Text box heights: the glance fonts' line heights (the SDK's generated Poppins tables give
// SemiBold 20 as 25 px and Regular 18 as 23).
constexpr int16_t kHead = 25;   ///< SEMIBOLD_20
constexpr int16_t kLine = 23;   ///< REGULAR_18
constexpr int16_t kTwoLineHeight = kHead + kLine;   ///< 48

// The strip under the words: seven pips and the plan's bar, 8 px tall, 3 px below the words.
constexpr int16_t kStripTop = kTwoLineHeight + 3;
constexpr int16_t kStripH   = 8;
constexpr int16_t kFullHeight = kStripTop + kStripH + 1;   ///< 60
constexpr int16_t kPipW     = 12;
constexpr int16_t kPipGap   = 4;
constexpr int16_t kPipsW    = 7 * kPipW + 6 * kPipGap;   ///< 108
constexpr int16_t kBarGap   = 14;
constexpr int16_t kBarW     = 56;
constexpr int16_t kStripW   = kPipsW + kBarGap + kBarW;  ///< 178

constexpr int kCandidates = 4;

/// The session's name on the glance: the app's short name, but "Easy" for an easy run with strides
/// ("Easy+str" is cryptic here; the line under it says "40 min + strides").
const char* glanceName(Kind k)
{
    return k == Kind::EasyStrides ? "Easy" : PlanText::shortName(k);
}

/// A set of wordings, longest first.
struct Choices {
    char    s[kCandidates][GLANCE_TEXT_SIZE + 1] {};
    uint8_t n = 0;

    void add(const char* fmt, const char* a = "", const char* b = "", const char* c = "")
    {
        if (n < kCandidates) {
            std::snprintf(s[n], sizeof(s[n]), fmt, a, b, c);
            ++n;
        }
    }
};

/// The first wording that fits @p limit px in @p table, else the last (the shortest) cut from its
/// end until it does, so a long string in the file never runs off the face.
void pick(const Choices& c, const uint8_t (&table)[95], int limit, char* out)
{
    const char* best = c.n > 0 ? c.s[c.n - 1] : "";
    for (uint8_t i = 0; i < c.n; ++i) {
        if (Fonts::width(c.s[i], table) <= limit) {
            best = c.s[i];
            break;
        }
    }
    std::snprintf(out, GLANCE_TEXT_SIZE + 1, "%s", best);
    for (size_t n = std::strlen(out); n > 0 && Fonts::width(out, table) > limit; --n) {
        out[n - 1] = '\0';
    }
}

/// "Tomorrow: Cruise 3 x 10 min", "Wed: Easy 45 min", or the shorter forms of them.
void nextLine(const View& v, Choices& c)
{
    char day[12];
    std::snprintf(day, sizeof(day), "%s", v.nextTomorrow ? "Tomorrow" : TimeText::weekdayName(v.nextWeekday));
    const char* name = glanceName(v.nextKind);
    if (v.nextWhat[0] != '\0') {
        c.add("%s: %s %s", day, name, v.nextWhat);
    }
    c.add("%s: %s", day, name);
    c.add("%s", name);
}

void wordChoices(const View& v, Choices& head, Choices& line, uint8_t& headColour)
{
    using S = TodayFile::State;
    headColour = GLANCE_COLOR_WHITE;
    const char* name = glanceName(v.kind);
    switch (v.state) {
    case S::Session:
        head.add("Today: %s", name);
        head.add("%s", name);
        if (v.pace[0] != '\0') {
            line.add("%s @ %s", v.what, v.pace);
        }
        line.add("%s", v.what);
        break;
    case S::Done:
        headColour = GLANCE_COLOR_GREEN;
        head.add("Done: %s", name);
        head.add("%s", name);
        if (v.hasNext) {
            nextLine(v, line);
        } else {
            line.add("Well done");
        }
        break;
    case S::Skipped:
        headColour = GLANCE_COLOR_YELLOW_DARK;
        head.add("Skipped: %s", name);
        head.add("%s", name);
        if (v.hasNext) {
            nextLine(v, line);
        } else {
            line.add("Rest up");
        }
        break;
    case S::Rest:
        head.add("Rest day");
        if (v.hasNext) {
            nextLine(v, line);
        } else {
            line.add("Rest up");
        }
        break;
    case S::Paused:
        head.add("Plan paused");
        line.add("Resume in the app");
        break;
    case S::BeforeStart: {
        head.add("Plan starts");
        char d[24];
        TimeText::dayDate(v.startDate, d, sizeof(d));
        line.add("%s", d);
        break;
    }
    case S::Complete:
        head.add("Plan complete");
        line.add("Well done");
        break;
    case S::Stale:
        head.add("X Training");
        line.add("Open the app to update");
        line.add("Open the app");
        break;
    case S::NoPlan:
        head.add("X Training");
        line.add("Set a plan in UNA");
        line.add("No plan yet");
        break;
    case S::ClockUnset:
        head.add("X Training");
        line.add("Set the watch's date");
        line.add("Date not set");
        break;
    case S::NoFile:
        head.add("X Training");
        line.add("Open the app to start");
        line.add("Open the app");
        break;
    }
}

Spec& add(Layout& out)
{
    Spec& s = out.items[out.count < Layout::kMax ? out.count++ : Layout::kMax - 1];
    s       = Spec {};
    return s;
}

void text(Layout& out, int16_t x, int16_t y, int16_t w, int16_t h, const char* str, uint8_t font, uint8_t colour,
          uint8_t align)
{
    Spec& s  = add(out);
    s.type   = Spec::Type::Text;
    s.x      = x;
    s.y      = y;
    s.w      = w;
    s.h      = h;
    s.font   = font;
    s.colour = colour;
    s.align  = align;
    std::snprintf(s.text, sizeof(s.text), "%s", str);
}

void rect(Layout& out, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t colour)
{
    Spec& s  = add(out);
    s.type   = Spec::Type::Rect;
    s.x      = x;
    s.y      = y;
    s.w      = w;
    s.h      = h;
    s.colour = colour;
}

uint8_t pipColour(Pip p)
{
    switch (p) {
    case Pip::Done:   return GLANCE_COLOR_GREEN;
    case Pip::Today:  return GLANCE_COLOR_WHITE;
    case Pip::ToCome: return GLANCE_COLOR_GRAY;
    case Pip::Missed: return GLANCE_COLOR_YELLOW_DARK;
    case Pip::Rest:
    case Pip::None:
        break;
    }
    return GLANCE_COLOR_GRAY_DARK;
}
}  // namespace

void words(const View& v, Words& out)
{
    out = Words{};
    Choices head, line;
    wordChoices(v, head, line, out.headColour);
    pick(head, Fonts::kSemiBold20, kBandW, out.head);
    pick(line, Fonts::kRegular18, kBandW, out.line);
}

void layout(const View& v, int16_t width, int16_t height, uint32_t maxControls, Layout& out)
{
    out = Layout {};
    Words w;
    words(v, w);

    // The band everything keeps to, centred; an area narrower than it uses its full width.
    const int16_t bw = width < kBandW ? width : kBandW;
    const int16_t bx = static_cast<int16_t>((width - bw) / 2);

    if (height < kTwoLineHeight || maxControls < 2) {
        // Tiny: one line, as much as fits.
        out.kind        = Layout::Kind::Tiny;
        const int16_t h = height < kHead ? height : kHead;
        text(out, bx, static_cast<int16_t>((height - h) / 2), bw, h, w.head, GLANCE_FONT_POPPINS_SEMIBOLD_20,
             w.headColour, GLANCE_ALIGN_H_CENTER);
        return;
    }
    if (!v.hasWeek || maxControls < kFullControls || width < kBandW || height < kFullHeight) {
        // Compact: the two lines, centred top to bottom.
        out.kind          = Layout::Kind::Compact;
        const int16_t top = static_cast<int16_t>((height - kTwoLineHeight) / 2);
        text(out, bx, top, bw, kHead, w.head, GLANCE_FONT_POPPINS_SEMIBOLD_20, w.headColour, GLANCE_ALIGN_H_CENTER);
        text(out, bx, static_cast<int16_t>(top + kHead), bw, kLine, w.line, GLANCE_FONT_POPPINS_REGULAR_18,
             GLANCE_COLOR_GRAY, GLANCE_ALIGN_H_CENTER);
        return;
    }

    // Full: the two lines, then the strip, centred top to bottom.
    out.kind          = Layout::Kind::Full;
    const int16_t top = static_cast<int16_t>((height - kFullHeight) / 2);
    text(out, bx, top, bw, kHead, w.head, GLANCE_FONT_POPPINS_SEMIBOLD_20, w.headColour, GLANCE_ALIGN_H_CENTER);
    text(out, bx, static_cast<int16_t>(top + kHead), bw, kLine, w.line, GLANCE_FONT_POPPINS_REGULAR_18,
         GLANCE_COLOR_GRAY, GLANCE_ALIGN_H_CENTER);
    const int16_t sy = static_cast<int16_t>(top + kStripTop);
    const int16_t sx = static_cast<int16_t>((width - kStripW) / 2);
    for (int i = 0; i < 7; ++i) {
        rect(out, static_cast<int16_t>(sx + i * (kPipW + kPipGap)), sy, kPipW, kStripH, pipColour(v.pip[i]));
    }
    // How far through the plan: the weeks begun, of all.
    const int16_t bx0 = static_cast<int16_t>(sx + kPipsW + kBarGap);
    rect(out, bx0, sy, kBarW, kStripH, GLANCE_COLOR_GRAY_DARK);
    int32_t fill = 0;
    if (v.weeks > 0 && v.weekNo > 0) {
        fill = static_cast<int32_t>(kBarW) * (v.weekNo > v.weeks ? v.weeks : v.weekNo) / v.weeks;
        if (fill < 2) {
            fill = 2;
        }
    }
    if (fill > 0) {
        rect(out, bx0, sy, static_cast<int16_t>(fill), kStripH, GLANCE_COLOR_WHITE);
    }
}

}  // namespace TrainingGlance
