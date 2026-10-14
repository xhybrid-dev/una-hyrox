/**
 * glance_preview: print the glance's controls for a few sample logs, one per
 * line, for docs/experiments/glance_preview.py to draw. The PC simulator
 * cannot run glances, so this is how the layout is seen before the watch.
 * Each panel runs the real code: a Trend fed weigh-ins, GlanceView, then
 * GlanceLayout. The logs are made up.
 *
 *   glance_preview <width> <height> <max controls>
 */

#include <cstdio>
#include <cstdlib>

#include "Calendar.hpp"
#include "GlanceLayout.hpp"
#include "GlanceView.hpp"
#include "Trend.hpp"

using namespace XWeighIn;

namespace
{
const int32_t kToday = daysFromCivil(2026, 10, 12);

/// Day-to-day swings, in 0.01 kg, repeating: water, salt, a late meal.
const int32_t kWobble[] = { 0, 30, -20, 10, 60, -10, 20, -30, 40, 0, -20, 50, 10, -40 };

/// A log of @p days days ending @p daysAgo days before today: 0.05 kg a day
/// down from @p start, plus the swings, with every @p skipEvery-th day missed
/// (0: none).
void feed(Trend& t, int32_t start, int days, int daysAgo, int skipEvery)
{
    t.reset();
    const int32_t first = kToday - daysAgo - days + 1;
    for (int i = 0; i < days; ++i) {
        if (skipEvery > 0 && i % skipEvery == skipEvery - 1 && i != days - 1) {
            continue;
        }
        Entry e;
        e.day        = first + i;
        e.minute     = 7 * 60;
        e.hundredths = (start - 5 * i + kWobble[i % 14]) / 10 * 10;   // 0.1 kg steps, as entered
        t.onEntry(e);
    }
}

void dump(const char* name, const Trend& t, bool clockSet, int16_t w, int16_t h, uint32_t n)
{
    Glance::View v;
    Glance::makeView(t, kToday, clockSet, v);
    Glance::Layout l;
    Glance::layout(v, w, h, n, l);
    std::printf("state %s %d %d\n", name, w, h);
    for (uint8_t i = 0; i < l.count; ++i) {
        const Glance::Spec& s = l.items[i];
        switch (s.type) {
            case Glance::Spec::Type::Text:
                std::printf("text %d %d %d %d %u %u %u %s\n", s.x, s.y, s.w, s.h, s.font, s.colour, s.align, s.text);
                break;
            case Glance::Spec::Type::Rect:
                std::printf("rect %d %d %d %d %u\n", s.x, s.y, s.w, s.h, s.colour);
                break;
        }
    }
}
} // namespace

int main(int argc, char** argv)
{
    const int16_t  w = static_cast<int16_t>(argc > 1 ? std::atoi(argv[1]) : 240);
    const int16_t  h = static_cast<int16_t>(argc > 2 ? std::atoi(argv[2]) : 60);
    const uint32_t n = static_cast<uint32_t>(argc > 3 ? std::atoi(argv[3]) : 32);

    static Trend t;   // about 1.2 KB
    feed(t, 8400, 42, 0, 9);
    dump("Six weeks, weighed in today", t, true, w, h, n);
    feed(t, 8240, 3, 0, 0);
    dump("The first three days", t, true, w, h, n);
    feed(t, 8400, 30, 5, 0);
    dump("Back after five days off", t, true, w, h, n);
    feed(t, 8400, 30, 21, 0);
    dump("Not weighed for three weeks", t, true, w, h, n);
    t.reset();
    dump("No weigh-ins yet", t, true, w, h, n);
    feed(t, 8400, 42, 0, 9);
    dump("Compact (16 controls)", t, true, w, h, Glance::kFullControls - 1);
    dump("Tiny (a 30 px area)", t, true, w, 30, n);
    return 0;
}
