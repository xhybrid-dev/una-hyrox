/**
 * Host tests for the glance layout, as HybridX Streak's: whatever area and
 * control budget the watch reports, every control fits inside it, the budget
 * is kept, and every text fits a glance text control: at most
 * GLANCE_TEXT_SIZE bytes, plain ASCII in a face that has letters, and, in the
 * full layout, no wider than its box in the real Poppins widths
 * (support/GlanceFontWidths.hpp). Line controls cannot occur: Spec has no
 * such type (they do not draw on the watch).
 */

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "GlanceLayout.hpp"
#include "support/GlanceFontWidths.hpp"

using XWeighIn::Glance::kFullControls;
using XWeighIn::Glance::kGlanceDays;
using XWeighIn::Glance::Layout;
using XWeighIn::Glance::Spec;
using XWeighIn::Glance::View;

namespace
{

/// A view of @p days weigh-ins ending today, falling 0.1 kg a day from
/// @p start, with no weigh-in on the days in @p missed. The trend is 0.5 kg
/// under the start.
View falling(int32_t start, uint8_t days, int32_t weekChange, std::vector<int> missed = {})
{
    View v;
    v.hasData       = true;
    v.trend         = start - 50;
    v.hasWeekChange = days >= 7;
    v.weekChange    = weekChange;
    v.lastDay       = 20738;
    v.endDay        = 20738;
    for (uint8_t i = 0; i < days && i < kGlanceDays; ++i) {
        const uint8_t slot = static_cast<uint8_t>(kGlanceDays - days + i);
        v.weight[slot]     = static_cast<uint16_t>(start - 10 * i);
    }
    for (int m : missed) {
        v.weight[m] = 0;
    }
    return v;
}

std::vector<std::pair<std::string, View>> views()
{
    View heavy = falling(50000, 14, -30);   // the most the log takes, 500.00 kg
    heavy.trend = 50000;
    View huge  = falling(30000, 14, 12345);   // a typo of a log: +123 kg in a week
    View stale = falling(8200, 10, -40);
    for (uint16_t& w : stale.weight) {
        w = 0;   // the last weigh-in more than 14 days ago
    }
    return {
        { "no log", View {} },
        { "one day", falling(8240, 1, 0) },
        { "three days", falling(8240, 3, 0) },
        { "two weeks", falling(8240, 14, -42) },
        { "with gaps", falling(8240, 14, 5, { 2, 3, 9, 13 }) },
        { "big loss", falling(12000, 14, -1040) },
        { "heavy", heavy },
        { "huge change", huge },
        { "stale", stale },
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

/// The middle of the area the glances list lines up with: its separators run
/// across the middle 172 px of the watch's 240, and its scroll arc covers both
/// edges (NOTES, Gate W3).
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
        EXPECT_GE(s.x, 0) << what;
        EXPECT_GE(s.y, 0) << what;
        EXPECT_GT(s.w, 0) << what << " control " << int(i);
        EXPECT_GT(s.h, 0) << what << " control " << int(i);
        EXPECT_LE(s.x + s.w, w) << what << " control " << int(i);
        EXPECT_LE(s.y + s.h, h) << what << " control " << int(i) << " '" << s.text << "'";
        if (s.type == Spec::Type::Text) {
            EXPECT_GT(std::strlen(s.text), 0u) << what;
            EXPECT_LE(std::strlen(s.text), static_cast<size_t>(GLANCE_TEXT_SIZE)) << what;
            for (const char* c = s.text; *c; ++c) {
                EXPECT_TRUE(*c >= 0x20 && *c <= 0x7E) << what << " non-ASCII in '" << s.text << "'";
            }
            // The 10 point face is digits only; words in it draw as '?'.
            EXPECT_NE(s.font, GLANCE_FONT_POPPINS_MEDIUM_10) << what << " '" << s.text << "'";
            const int tw = widthOf(s);
            EXPECT_GE(tw, 0) << what << " unmeasured face " << int(s.font);
            // The full layout (the watch's 240x60) must show every word; the
            // others are fallbacks for areas no watch has reported.
            if (l.kind == Layout::Kind::Full) {
                EXPECT_LE(tw, s.w) << what << " '" << s.text << "' is " << tw << " px in " << s.w;
            }
        }
    }
}

Layout full(const View& v)
{
    Layout l;
    XWeighIn::Glance::layout(v, 240, 60, 32, l);
    return l;
}

/// The bars of a full layout: every rect after the baseline (items[2]).
std::vector<Spec> barsOf(const Layout& l)
{
    std::vector<Spec> out;
    for (uint8_t i = 3; i < l.count; ++i) {
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
                XWeighIn::Glance::layout(v, a[0], a[1], b, l);
                expectFits(l, a[0], a[1], b,
                           name + " " + std::to_string(a[0]) + "x" + std::to_string(a[1]) + "/" + std::to_string(b));
            }
        }
    }
}

TEST(GlanceLayout, TheWatchGetsTheFullLayout)
{
    const Layout l = full(falling(8240, 14, -42));
    ASSERT_EQ(l.kind, Layout::Kind::Full);
    EXPECT_EQ(l.count, kFullControls);
    // In line with the list's separators, x = 34 to 206: the words start at
    // the left one and the bars end at the right one.
    EXPECT_EQ(l.items[0].x, 34);
    EXPECT_EQ(l.items[1].x, 34);
    EXPECT_EQ(l.items[2].x + l.items[2].w, 206);
    ASSERT_EQ(l.items[0].type, Spec::Type::Text);
    EXPECT_STREQ(l.items[0].text, "81.9 kg");
    EXPECT_EQ(l.items[0].font, GLANCE_FONT_POPPINS_SEMIBOLD_25);
    ASSERT_EQ(l.items[1].type, Spec::Type::Text);
    EXPECT_STREQ(l.items[1].text, "-0.4 kg/wk");
    EXPECT_EQ(l.items[2].type, Spec::Type::Rect);   // the baseline
    EXPECT_EQ(l.items[2].colour, GLANCE_COLOR_GRAY_DARK);
}

TEST(GlanceLayout, TheWords)
{
    EXPECT_STREQ(full(falling(8240, 3, 0)).items[1].text, "-- kg/wk");
    EXPECT_STREQ(full(falling(8240, 14, 0)).items[1].text, "0.0 kg/wk");
    EXPECT_STREQ(full(falling(8240, 14, 35)).items[1].text, "+0.4 kg/wk");
    EXPECT_STREQ(full(falling(12000, 14, -1040)).items[1].text, "-10.4 kg/wk");
    EXPECT_STREQ(full(falling(30000, 14, 12345)).items[1].text, "+123 kg/wk");

    Layout empty;
    XWeighIn::Glance::layout(View {}, 240, 60, 32, empty);
    EXPECT_EQ(empty.kind, Layout::Kind::Compact);
    ASSERT_EQ(empty.count, 2);
    EXPECT_STREQ(empty.items[0].text, "X Weigh-in");
    EXPECT_STREQ(empty.items[1].text, "No weigh-ins yet");

    Layout tiny;
    XWeighIn::Glance::layout(falling(8240, 14, -42), 240, 30, 32, tiny);
    EXPECT_EQ(tiny.kind, Layout::Kind::Tiny);
    ASSERT_EQ(tiny.count, 1);
    EXPECT_STREQ(tiny.items[0].text, "81.9 kg");
}

TEST(GlanceLayout, ABarPerWeighInStandingOnTheBaseline)
{
    const View   v    = falling(8240, 14, 5, { 2, 3, 9 });
    const Layout l    = full(v);
    const Spec&  base = l.items[2];
    const auto   bars = barsOf(l);
    ASSERT_EQ(bars.size(), 11u);   // 14 days, 3 missed
    int prevRight = -1;
    for (const Spec& b : bars) {
        EXPECT_EQ(b.type, Spec::Type::Rect);
        EXPECT_EQ(b.y + b.h, base.y) << "x " << b.x;   // stands on the baseline
        EXPECT_GE(b.h, 2);
        EXPECT_GE(b.x, base.x);
        EXPECT_LE(b.x + b.w, base.x + base.w);
        EXPECT_GT(b.x, prevRight);                     // in day order, no overlap
        prevRight = b.x + b.w - 1;
        // Clear of the words' column.
        EXPECT_GE(b.x, l.items[1].x + l.items[1].w);
    }
    // Missed days leave their slots empty: after days 2 and 3, the next bar
    // is three slots on from day 1's (days 0 and 1 are one slot apart).
    EXPECT_EQ(bars[2].x - bars[1].x, 3 * (bars[1].x - bars[0].x));
}

TEST(GlanceLayout, HeavierIsTallerAndTheNewestIsWhite)
{
    const auto bars = barsOf(full(falling(8240, 14, -42)));
    ASSERT_EQ(bars.size(), static_cast<size_t>(kGlanceDays));
    for (size_t i = 1; i < bars.size(); ++i) {
        EXPECT_LE(bars[i].h, bars[i - 1].h) << i;   // a falling weight
        EXPECT_EQ(bars[i - 1].colour, GLANCE_COLOR_GRAY);
    }
    EXPECT_LT(bars.back().h, bars.front().h);
    EXPECT_EQ(bars.back().colour, GLANCE_COLOR_WHITE);
}

TEST(GlanceLayout, TheNewestBarIsWhiteEvenBeforeToday)
{
    View v = falling(8240, 14, -42);
    v.weight[kGlanceDays - 1] = 0;   // not weighed today yet
    const auto bars = barsOf(full(v));
    ASSERT_EQ(bars.size(), static_cast<size_t>(kGlanceDays - 1));
    EXPECT_EQ(bars.back().colour, GLANCE_COLOR_WHITE);
    EXPECT_EQ(bars[bars.size() - 2].colour, GLANCE_COLOR_GRAY);
}

TEST(GlanceLayout, ASteadyWeekLooksSteady)
{
    // A 0.2 kg wobble on a scale at least 2 kg tall stays small.
    View v = falling(8240, 14, 0);
    for (uint8_t i = 0; i < kGlanceDays; ++i) {
        v.weight[i] = static_cast<uint16_t>(i % 2 ? 8240 : 8220);
    }
    const auto bars = barsOf(full(v));
    ASSERT_EQ(bars.size(), static_cast<size_t>(kGlanceDays));
    for (const Spec& b : bars) {
        EXPECT_LE(std::abs(b.h - bars.front().h), 6) << b.x;
    }
}

TEST(GlanceLayout, NoRecentWeighInsLeaveTheBaselineAlone)
{
    View v = falling(8200, 10, -40);
    for (uint16_t& w : v.weight) {
        w = 0;
    }
    const Layout l = full(v);
    EXPECT_EQ(l.kind, Layout::Kind::Full);
    EXPECT_EQ(l.count, 3);
    EXPECT_STREQ(l.items[0].text, "81.5 kg");
}

TEST(GlanceLayout, FewControlsGiveTheCompactLayout)
{
    Layout l;
    XWeighIn::Glance::layout(falling(8240, 14, -42), 240, 60, kFullControls - 1, l);
    EXPECT_EQ(l.kind, Layout::Kind::Compact);
    ASSERT_EQ(l.count, 2);
    EXPECT_STREQ(l.items[0].text, "81.9 kg");
    EXPECT_STREQ(l.items[1].text, "-0.4 kg/wk");
}
