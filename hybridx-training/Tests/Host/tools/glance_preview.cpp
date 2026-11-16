// glance_preview: the controls X Training's glance draws for a few sample plans, printed one
// per line for docs/experiments/glance_preview.py to draw (the PC simulator cannot run
// glances). The real TodayBuild, GlanceView and GlanceLayout code on a real plan (the 10K
// plan, level 2, a 22:00 5K), so what it draws is what the watch is told to draw.
//
//   glance_preview [width height controls]      the lines:
//   state <name> <w> <h>
//   text x y w h font colour align <text>
//   rect x y w h colour

#include "GlanceLayout.hpp"
#include "TodayBuild.hpp"
#include "data/PlanData.hpp"

#include <cstdio>
#include <cstdlib>
#include <memory>

using namespace XTraining;
using namespace TrainingGlance;

namespace {

constexpr Ymd kStart = 20270104u;   // a Monday

struct Scene {
    PlanState st;
    Paces paces;
    std::unique_ptr<SDK::Workout::Program> scratch{new SDK::Workout::Program()};
    bool imperial = false;
    explicit Scene(PlanId id = PlanId::TenK, uint8_t level = 2, uint32_t fiveK = 22 * 60)
    {
        const PlanDef& def = *PlanData::find(id);
        st.setup.plan = id;
        st.setup.level = level;
        st.setup.fiveKSec = fiveK;
        st.setup.fiveKDate = kStart;
        st.calendar = PlanCalendar::make({kStart, 0, def.weekCount, def.minWeeks});
        paces = PaceModel::compute({fiveK, 0}).paces;
    }
    TodayData file(Ymd today)
    {
        TodayData d;
        TodayBuild::fill(st, &paces, imperial, today, scratch.get(), d);
        return d;
    }
};

void draw(const char* name, const View& v, int16_t w, int16_t h, uint32_t n)
{
    static TrainingGlance::Layout l;
    TrainingGlance::layout(v, w, h, n, l);
    std::printf("state %s %d %d\n", name, w, h);
    for (uint8_t i = 0; i < l.count; ++i) {
        const TrainingGlance::Spec& s = l.items[i];
        if (s.type == TrainingGlance::Spec::Type::Text) {
            std::printf("text %d %d %d %d %u %u %u %s\n", s.x, s.y, s.w, s.h, s.font, s.colour, s.align, s.text);
        } else {
            std::printf("rect %d %d %d %d %u\n", s.x, s.y, s.w, s.h, s.colour);
        }
    }
}

TodayDay* row(TodayData& d, Ymd date) { return const_cast<TodayDay*>(d.find(date)); }

}  // namespace

int main(int argc, char** argv)
{
    const int16_t  w = static_cast<int16_t>(argc > 1 ? std::atoi(argv[1]) : 240);
    const int16_t  h = static_cast<int16_t>(argc > 2 ? std::atoi(argv[2]) : 60);
    const uint32_t n = static_cast<uint32_t>(argc > 3 ? std::atoi(argv[3]) : 32);
    static View v;

    // Week 6, Tuesday 8 February 2027: intervals. Monday was a rest day; the week before was done.
    Scene s;
    const Ymd tue = 20270209u;
    TodayData d = s.file(tue);
    makeView(&d, tue, v);
    draw("Intervals today", v, w, h, n);

    // The same day after the run is saved: done, and tomorrow's session is next.
    row(d, tue)->status = TodayStatus::Done;
    makeView(&d, tue, v);
    draw("Done today, the next session", v, w, h, n);

    // Wednesday: an easy run, with Tuesday's done behind it, then a rest day.
    TodayData wed = s.file(20270210u);
    row(wed, tue)->status = TodayStatus::Done;
    makeView(&wed, 20270210u, v);
    draw("Easy run, a day after", v, w, h, n);

    // Thursday: a rest day, Friday's cruise intervals tomorrow.
    TodayData thu = wed;
    row(thu, 20270210u)->status = TodayStatus::Done;
    makeView(&thu, 20270211u, v);
    draw("Rest day", v, w, h, n);

    // The same Tuesday for a runner in miles.
    Scene mi;
    mi.imperial = true;
    TodayData miles = mi.file(tue);
    makeView(&miles, tue, v);
    draw("Intervals today, in miles", v, w, h, n);

    // A long run with a fast finish, in the half plan's peak.
    Scene half(PlanId::Half, 3, 19 * 60);
    TodayData peak = half.file(20270404u);
    {
        // The first Sunday of the plan's second half whose session is the fast-finish long run.
        Ymd day = 20270307u;
        for (int i = 0; i < 70 && Date::weekday(day) != Weekday::Sun; ++i) { day = Date::addDays(day, 1); }
        for (int i = 0; i < 12; ++i, day = Date::addDays(day, 7)) {
            TodayData t = half.file(day);
            if (t.find(day) != nullptr && t.find(day)->kind == Kind::LongFinish) {
                peak = t;
                // The week so far done: its sessions before the Sunday.
                for (uint8_t i = 0; i < peak.count; ++i) {
                    if (peak.day[i].date < day && peak.day[i].kind != Kind::Rest) {
                        peak.day[i].status = TodayStatus::Done;
                    }
                }
                makeView(&peak, day, v);
                draw("Long run, fast finish", v, w, h, n);
                break;
            }
        }
    }

    // A session skipped, and a missed one in the week.
    TodayData skipped = s.file(tue);
    row(skipped, tue)->status = TodayStatus::Skipped;
    makeView(&skipped, tue, v);
    draw("Skipped today", v, w, h, n);

    // A week in which things went wrong: Tuesday missed, today's easy run still to do.
    TodayData rough = s.file(20270212u);   // Friday
    row(rough, tue)->status = TodayStatus::Missed;
    row(rough, 20270210u)->status = TodayStatus::Done;
    makeView(&rough, 20270212u, v);
    draw("Friday, Tuesday missed", v, w, h, n);

    // The states with no session of their own.
    TodayData paused = s.file(tue);
    paused.pausedFrom = 20270208u;
    makeView(&paused, tue, v);
    draw("Plan paused", v, w, h, n);

    TodayData early = s.file(20261231u);
    makeView(&early, 20261231u, v);
    draw("Plan starts Monday", v, w, h, n);

    TodayData stale = s.file(tue);
    makeView(&stale, 20270301u, v);
    draw("A fortnight on, the app not opened", v, w, h, n);

    const Ymd end = Date::addDays(kStart, s.st.calendar.weeks * 7 - 1);
    TodayData last = s.file(end);
    makeView(&last, Date::addDays(end, 1), v);
    draw("Plan complete", v, w, h, n);

    const TodayData none;
    makeView(&none, tue, v);
    draw("No plan", v, w, h, n);

    makeView(nullptr, tue, v);
    draw("The app not opened yet", v, w, h, n);

    makeView(&d, 0, v);
    draw("The watch's date not set", v, w, h, n);
    return 0;
}
