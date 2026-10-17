/**
 * Host tests for the VO2max glance layout, as X Weigh-in's and HybridX
 * Streak's: whatever area and control budget the watch reports, every control
 * fits inside it and the middle band, the budget is kept, and every text fits
 * a glance text control: at most GLANCE_TEXT_SIZE bytes, plain ASCII in a face
 * that has letters, and, in the full layout, no wider than its box in the real
 * Poppins widths (support/GlanceFontWidths.hpp). Line controls cannot occur:
 * Spec has no such type (they do not draw on the watch).
 */

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "GlanceLayout.hpp"
#include "support/GlanceFontWidths.hpp"

using RunVo2::Glance::kFullControls;
using RunVo2::Glance::kGlanceRuns;
using RunVo2::Glance::Layout;
using RunVo2::Glance::Spec;
using RunVo2::Glance::View;

namespace
{

/// A view of @p n runs, the trend stepping by @p step from @p first.
View runs(uint8_t n, uint16_t first, int step)
{
    View v;
    v.hasData = n > 0;
    v.runs    = n;
    v.count   = n;
    for (uint8_t i = 0; i < n; ++i) {
        v.trend[i] = static_cast<uint16_t>(first + step * i);
    }
    if (n > 0) {
        v.vo2x10    = v.trend[n - 1];
        v.beforeX10 = n > 1 ? v.trend[n - 2] : 0;
    }
    return v;
}

std::vector<std::pair<std::string, View>> views()
{
    View jump = runs(2, 300, 0);
    jump.vo2x10    = 950;   // the most the app keeps, after a first run at 15.0
    jump.beforeX10 = 150;
    jump.trend[0]  = 150;
    jump.trend[1]  = 950;
    View slip = runs(10, 600, -3);
    slip.beforeX10 = slip.vo2x10 + 99;   // -9.9, the widest line with a decimal
    View wide = runs(10, 888, 0);
    wide.beforeX10 = 150;                // -73
    return {
        { "no runs", View {} },
        { "one run", runs(1, 523, 0) },
        { "three runs", runs(3, 505, 7) },
        { "ten rising", runs(10, 480, 5) },
        { "ten falling", runs(10, 560, -4) },
        { "steady", runs(10, 523, 0) },
        { "big jump", jump },
        { "slip", slip },
        { "wide", wide },
    };
}

/// The text's width in its face, or -1 for a face these tests do not know.
int widthOf(const Spec& s)
{
    const uint8_t* table = nullptr;
    switch (s.font) {
        case GLANCE_FONT_POPPINS_REGULAR_18: table = GlanceFontWidths::kRegular18; break;
        case GLANCE_FONT_POPPINS_MEDIUM_18: table = GlanceFontWidths::kMedium18; break;
        case GLANCE_FONT_POPPINS_SEMIBOLD_20: table = GlanceFontWidths::kSemiBold20; break;
        case GLANCE_FONT_POPPINS_SEMIBOLD_25: table = GlanceFontWidths::kSemiBold25; break;
        default: return -1;
    }
    int w = 0;
    for (const char* c = s.text; *c; ++c) {
        w += (*c >= 0x20 && *c <= 0x7E) ? table[*c - 0x20] : 0;
    }
    return w;
}

/// The middle of the area the glances list lines up with (X Weigh-in NOTES,
/// Gate W3).
constexpr int16_t kBand = 172;

void expectFits(const Layout& l, int16_t w, int16_t h, uint32_t maxControls, const std::string& what)
{
    EXPECT_LE(l.count, maxControls) << what;
    EXPECT_LE(l.count, Layout::kMax) << what;
    const int16_t bandL = w > kBand ? static_cast<int16_t>((w - kBand) / 2) : 0;
    const int16_t bandR = w > kBand ? static_cast<int16_t>(bandL + kBand) : w;
    for (uint8_t i = 0; i < l.count; ++i) {
        const Spec& s = l.items[i];
        EXPECT_GE(s.x, bandL) << what << " control " << int(i) << " left of the band";
        EXPECT_LE(s.x + s.w, bandR) << what << " control " << int(i) << " right of the band";
        EXPECT_GE(s.y, 0) << what;
        EXPECT_GT(s.w, 0) << what << " control " << int(i);
        EXPECT_GT(s.h, 0) << what << " control " << int(i);
        EXPECT_LE(s.y + s.h, h) << what << " control " << int(i) << " '" << s.text << "'";
        if (s.type == Spec::Type::Text) {
            EXPECT_GT(std::strlen(s.text), 0u) << what;
            EXPECT_LE(std::strlen(s.text), static_cast<size_t>(GLANCE_TEXT_SIZE)) << what;
            for (const char* c = s.text; *c; ++c) {
                EXPECT_TRUE(*c >= 0x20 && *c <= 0x7E) << what << " non-ASCII in '" << s.text << "'";
            }
            EXPECT_NE(s.font, GLANCE_FONT_POPPINS_MEDIUM_10) << what << " '" << s.text << "'";
            const int tw = widthOf(s);
            EXPECT_GE(tw, 0) << what << " unmeasured face " << int(s.font);
            // The full layout (the watch's 240x60) must show every word; the
            // others are fallbacks for areas no watch has reported, apart
            // from the empty state, which the watch shows compact.
            if (l.kind == Layout::Kind::Full || (w == 240 && h == 60)) {
                EXPECT_LE(tw, s.w) << what << " '" << s.text << "' is " << tw << " px in " << s.w;
            }
        }
    }
}

Layout full(const View& v)
{
    Layout l;
    RunVo2::Glance::layout(v, 240, 60, 32, l);
    return l;
}

/// The bars of a full layout: every rect after the baseline (items[3]).
std::vector<Spec> barsOf(const Layout& l)
{
    std::vector<Spec> out;
    for (uint8_t i = 4; i < l.count; ++i) {
        out.push_back(l.items[i]);
    }
    return out;
}

} // namespace

TEST(GlanceLayout, FitsEveryAreaAndBudget)
{
    const int16_t  areas[][2] = { { 240, 60 }, { 240, 48 }, { 240, 40 }, { 200, 60 },
                                  { 240, 80 }, { 180, 60 }, { 300, 60 }, { 240, 20 } };
    const uint32_t budgets[]  = { 32, kFullControls, kFullControls - 1, 8, 2, 1 };
    for (const auto& [name, v] : views()) {
        for (const auto& a : areas) {
            for (uint32_t b : budgets) {
                Layout l;
                RunVo2::Glance::layout(v, a[0], a[1], b, l);
                expectFits(l, a[0], a[1], b,
                           name + " " + std::to_string(a[0]) + "x" + std::to_string(a[1]) + "/" + std::to_string(b));
            }
        }
    }
}

TEST(GlanceLayout, TheWatchGetsTheFullLayout)
{
    const Layout l = full(runs(10, 480, 5));
    ASSERT_EQ(l.kind, Layout::Kind::Full);
    EXPECT_EQ(l.count, kFullControls);
    // In line with the list's separators, x = 34 to 206.
    EXPECT_EQ(l.items[0].x, 34);
    EXPECT_EQ(l.items[2].x, 34);
    EXPECT_EQ(l.items[3].x + l.items[3].w, 206);
    EXPECT_STREQ(l.items[0].text, "52.5");
    EXPECT_EQ(l.items[0].font, GLANCE_FONT_POPPINS_SEMIBOLD_25);
    EXPECT_STREQ(l.items[1].text, "VO2");
    EXPECT_EQ(l.items[1].x, l.items[0].x + l.items[0].w);   // just after the figure
    EXPECT_STREQ(l.items[2].text, "+0.5 last run");
    EXPECT_EQ(l.items[3].type, Spec::Type::Rect);   // the baseline
    EXPECT_EQ(l.items[3].colour, GLANCE_COLOR_GRAY_DARK);
}

TEST(GlanceLayout, TheWords)
{
    EXPECT_STREQ(full(runs(1, 523, 0)).items[2].text, "First run");
    EXPECT_STREQ(full(runs(3, 523, 0)).items[2].text, "0.0 last run");
    EXPECT_STREQ(full(runs(3, 523, -4)).items[2].text, "-0.4 last run");
    EXPECT_STREQ(full(runs(2, 400, 99)).items[2].text, "+9.9 last run");
    EXPECT_STREQ(full(runs(2, 400, 125)).items[2].text, "+13 last run");   // 12.5, no decimal

    Layout empty;
    RunVo2::Glance::layout(View {}, 240, 60, 32, empty);
    EXPECT_EQ(empty.kind, Layout::Kind::Compact);
    ASSERT_EQ(empty.count, 2);
    EXPECT_STREQ(empty.items[0].text, "VO2max");
    EXPECT_STREQ(empty.items[1].text, "No runs yet");

    Layout tiny;
    RunVo2::Glance::layout(runs(10, 480, 5), 240, 30, 32, tiny);
    EXPECT_EQ(tiny.kind, Layout::Kind::Tiny);
    ASSERT_EQ(tiny.count, 1);
    EXPECT_STREQ(tiny.items[0].text, "52.5 VO2");
}

TEST(GlanceLayout, ABarPerRunNewestOnTheRight)
{
    const Layout l    = full(runs(4, 505, 7));
    const Spec&  base = l.items[3];
    const auto   bars = barsOf(l);
    ASSERT_EQ(bars.size(), 4u);
    int prevRight = -1;
    for (const Spec& b : bars) {
        EXPECT_EQ(b.type, Spec::Type::Rect);
        EXPECT_EQ(b.y + b.h, base.y) << "x " << b.x;   // stands on the baseline
        EXPECT_GE(b.h, 2);
        EXPECT_GE(b.x, base.x);
        EXPECT_LE(b.x + b.w, base.x + base.w);
        EXPECT_GT(b.x, prevRight);                     // in run order, no overlap
        prevRight = b.x + b.w - 1;
        EXPECT_GE(b.x, l.items[2].x + l.items[2].w);   // clear of the words
    }
    // The newest ends at the baseline's right end; earlier slots stay empty.
    EXPECT_EQ(bars.back().x + bars.back().w, base.x + base.w);
}

TEST(GlanceLayout, HigherIsTallerAndTheNewestIsWhite)
{
    const auto bars = barsOf(full(runs(10, 480, 5)));
    ASSERT_EQ(bars.size(), static_cast<size_t>(kGlanceRuns));
    for (size_t i = 1; i < bars.size(); ++i) {
        EXPECT_GE(bars[i].h, bars[i - 1].h) << i;
        EXPECT_EQ(bars[i - 1].colour, GLANCE_COLOR_GRAY);
    }
    EXPECT_GT(bars.back().h, bars.front().h);
    EXPECT_EQ(bars.back().colour, GLANCE_COLOR_WHITE);
}

TEST(GlanceLayout, ASteadyFigureLooksSteady)
{
    // A 0.2 wobble on a scale at least 4.0 tall stays small.
    View v = runs(10, 523, 0);
    for (uint8_t i = 0; i < kGlanceRuns; ++i) {
        v.trend[i] = static_cast<uint16_t>(i % 2 ? 525 : 523);
    }
    const auto bars = barsOf(full(v));
    ASSERT_EQ(bars.size(), static_cast<size_t>(kGlanceRuns));
    for (const Spec& b : bars) {
        EXPECT_LE(std::abs(b.h - bars.front().h), 3) << b.x;
        EXPECT_GT(b.h, 10) << b.x;   // mid-scale, not on the floor
    }
}

TEST(GlanceLayout, FewControlsGiveTheCompactLayout)
{
    Layout l;
    RunVo2::Glance::layout(runs(10, 480, 5), 240, 60, kFullControls - 1, l);
    EXPECT_EQ(l.kind, Layout::Kind::Compact);
    ASSERT_EQ(l.count, 2);
    EXPECT_STREQ(l.items[0].text, "52.5 VO2");
    EXPECT_STREQ(l.items[1].text, "+0.5 last run");
}
