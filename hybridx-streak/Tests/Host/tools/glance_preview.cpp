/**
 * glance_preview: print the glance's controls for a few states, one per line,
 * for docs/experiments/glance_preview.py to draw. The PC simulator cannot run
 * glances (NOTES S0), so this is how the layout is seen before the watch.
 *
 *   glance_preview <width> <height> <max controls>
 */

#include <cstdio>
#include <cstdlib>

#include "GlanceLayout.hpp"

namespace
{
void dump(const char* name, const Streak::HomeView& v, Glance::State st, int16_t w, int16_t h, uint32_t n)
{
    Glance::Layout l;
    Glance::layout(v, st, w, h, n, l);
    std::printf("state %s %d %d\n", name, w, h);
    for (uint8_t i = 0; i < l.count; ++i) {
        const Glance::Spec& s = l.items[i];
        switch (s.type) {
            case Glance::Spec::Type::Text:
                std::printf("text %d %d %d %d %u %u %u %s\n", s.x, s.y, s.w, s.h, s.font, s.colour, s.align, s.text);
                break;
            case Glance::Spec::Type::Line:
                std::printf("line %d %d %d %d %u\n", s.x, s.y, s.x2, s.y2, s.colour);
                break;
            case Glance::Spec::Type::Rect:
                std::printf("rect %d %d %d %d %u\n", s.x, s.y, s.w, s.h, s.colour);
                break;
        }
    }
}

Streak::HomeView view(uint16_t streak, uint16_t weeks, uint8_t sessions, uint8_t target, Streak::Mood mood,
                      uint8_t flags = 0)
{
    Streak::HomeView v;
    v.streakWeeks   = streak;
    v.weeksAchieved = weeks;
    v.sessions      = sessions;
    v.target        = target;
    v.daysLeft      = 2;
    v.mood          = mood;
    v.flags         = flags;
    return v;
}
} // namespace

int main(int argc, char** argv)
{
    const int16_t  w = static_cast<int16_t>(argc > 1 ? std::atoi(argv[1]) : 240);
    const int16_t  h = static_cast<int16_t>(argc > 2 ? std::atoi(argv[2]) : 60);
    const uint32_t n = static_cast<uint32_t>(argc > 3 ? std::atoi(argv[3]) : 32);
    dump("Climbing", view(7, 7, 2, 3, Streak::Mood::Climbing), Glance::State::Normal, w, h, n);
    dump("Week banked", view(12, 12, 3, 3, Streak::Mood::Done), Glance::State::Normal, w, h, n);
    dump("At risk", view(23, 40, 2, 4, Streak::Mood::AtRisk), Glance::State::Normal, w, h, n);
    dump("First week", view(0, 0, 1, 3, Streak::Mood::Trial, Streak::HomeView::kTrialWeek), Glance::State::Normal, w,
         h, n);
    dump("Shield waiting", view(9, 30, 0, 3, Streak::Mood::Climbing, Streak::HomeView::kDecisionPending),
         Glance::State::Normal, w, h, n);
    dump("Not opened yet", Streak::HomeView {}, Glance::State::NoStreak, w, h, n);
    dump("Compact (few controls)", view(7, 7, 2, 3, Streak::Mood::Climbing), Glance::State::Normal, w, h, 4);
    return 0;
}
