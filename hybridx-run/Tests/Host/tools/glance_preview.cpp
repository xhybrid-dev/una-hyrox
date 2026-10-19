/**
 * glance_preview: print the VO2max glance's controls for a few sample
 * histories, one per line, for docs/experiments/glance_preview.py to draw.
 * The PC simulator cannot run glances, so this is how the layout is seen
 * before the watch. Each panel runs the real code end to end: a Vo2History
 * fed runs, its shared file (toSharedJson), GlanceView reading that text,
 * then GlanceLayout. The runs are made up.
 *
 *   glance_preview <width> <height> <max controls>
 */

#include <cstdio>
#include <cstdlib>

#include "GlanceLayout.hpp"
#include "GlanceView.hpp"
#include "Vo2History.hpp"

using namespace RunVo2;

namespace
{
/// Run-to-run scatter, x 10, repeating: the method's noise.
const int kScatter[] = { 0, 9, -6, 4, 12, -8, 3, -11, 7, 0, -4, 10 };

/// @p n runs, the estimate moving @p step (x 10) a run from @p first, plus
/// the scatter, each run kept with @p windows windows.
void feed(Vo2History& h, int n, int first, int step, uint16_t windows)
{
    h.clear();
    for (int i = 0; i < n; ++i) {
        const int v = first + step * i + kScatter[i % 12];
        h.add(RunRecord{1790000000u + 86400u * 2u * static_cast<uint32_t>(i), static_cast<uint16_t>(v), windows});
    }
}

void dump(const char* name, const Vo2History& h, int16_t w, int16_t height, uint32_t n)
{
    char buf[Vo2History::kMaxSharedJsonBytes];
    const size_t len = h.toSharedJson(buf, sizeof(buf));
    Glance::View v;
    Glance::makeView(buf, len, v);
    Glance::Layout l;
    Glance::layout(v, w, height, n, l);
    std::printf("state %s %d %d\n", name, w, height);
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

    static Vo2History hist;
    feed(hist, 14, 480, 4, 20);
    dump("Ten runs, getting fitter", hist, w, h, n);
    feed(hist, 12, 545, -3, 20);
    dump("Ten runs, a break from training", hist, w, h, n);
    feed(hist, 10, 523, 0, 20);
    dump("Ten runs, steady", hist, w, h, n);
    feed(hist, 3, 505, 5, 20);
    dump("The first three runs", hist, w, h, n);
    feed(hist, 1, 512, 0, 20);
    dump("The first run", hist, w, h, n);
    hist.clear();
    dump("No runs yet", hist, w, h, n);
    feed(hist, 14, 480, 4, 20);
    dump("Compact (13 controls)", hist, w, h, Glance::kFullControls - 1);
    dump("Tiny (a 30 px area)", hist, w, 30, n);
    return 0;
}
