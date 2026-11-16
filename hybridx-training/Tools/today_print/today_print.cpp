// today_print: read a training_today.json with the core's own reader and say what the glance
// would pick for a day (TP6). For checking the file the app really wrote (docs/experiments/sim_plan.sh)
// and for looking at one.
//
//   today_print <training_today.json> [yyyymmdd] [--expect State]
//
// Prints the header, each row, and the state the glance shows on that day (default: today by
// the host's clock). Exit 0 if the file reads (and, with --expect, the state matches), 1 if not.

#include "PlanText.hpp"
#include "TodayFile.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

using namespace XTraining;

namespace {

const char* stateName(TodayFile::State s)
{
    switch (s) {
    case TodayFile::State::NoFile:      return "NoFile";
    case TodayFile::State::NoPlan:      return "NoPlan";
    case TodayFile::State::ClockUnset:  return "ClockUnset";
    case TodayFile::State::Stale:       return "Stale";
    case TodayFile::State::Paused:      return "Paused";
    case TodayFile::State::BeforeStart: return "BeforeStart";
    case TodayFile::State::Complete:    return "Complete";
    case TodayFile::State::Session:     return "Session";
    case TodayFile::State::Done:        return "Done";
    case TodayFile::State::Skipped:     return "Skipped";
    case TodayFile::State::Rest:        return "Rest";
    }
    return "?";
}

const char* statusName(TodayStatus s)
{
    switch (s) {
    case TodayStatus::ToCome:   return "to come";
    case TodayStatus::Done:     return "done";
    case TodayStatus::PartDone: return "part done";
    case TodayStatus::Missed:   return "missed";
    case TodayStatus::Skipped:  return "skipped";
    }
    return "?";
}

Ymd hostToday()
{
    const std::time_t t = std::time(nullptr);
    const std::tm* l = std::localtime(&t);
    return Date::make(l->tm_year + 1900, l->tm_mon + 1, l->tm_mday);
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: today_print <training_today.json> [yyyymmdd] [--expect State]\n");
        return 2;
    }
    Ymd today = hostToday();
    const char* expect = nullptr;
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--expect") == 0 && i + 1 < argc) {
            expect = argv[++i];
        } else {
            today = static_cast<Ymd>(std::strtoul(argv[i], nullptr, 10));
        }
    }
    std::FILE* f = std::fopen(argv[1], "rb");
    if (f == nullptr) {
        std::printf("no file: %s\n", argv[1]);
        return 1;
    }
    static char text[TodayFile::kMaxJsonBytes + 1];
    const size_t n = std::fread(text, 1, TodayFile::kMaxJsonBytes + 1, f);
    std::fclose(f);
    static TodayData d;
    if (!TodayFile::read(text, n, d)) {
        std::printf("the file does not read (%zu bytes)\n", n);
        return 1;
    }
    std::printf("plan %u, %u weeks, %lu to %lu, window %lu to %lu, paused from %lu, %u days, %zu bytes\n", d.plan,
                d.weeks, static_cast<unsigned long>(d.start), static_cast<unsigned long>(d.end),
                static_cast<unsigned long>(d.first), static_cast<unsigned long>(d.last),
                static_cast<unsigned long>(d.pausedFrom), d.count, n);
    for (uint8_t i = 0; i < d.count; ++i) {
        const TodayDay& r = d.day[i];
        std::printf("  %lu  week %-2u  %-10s %-9s %-18s %s\n", static_cast<unsigned long>(r.date), r.runWeek,
                    PlanText::shortName(r.kind), statusName(r.status), r.what, r.pace);
    }
    const TodayFile::Pick p = TodayFile::pick(&d, today);
    std::printf("on %lu the glance shows: %s", static_cast<unsigned long>(today), stateName(p.state));
    if (p.today != nullptr) {
        std::printf(", %s %s", PlanText::shortName(p.today->kind), p.today->what);
    }
    if (p.next != nullptr) {
        std::printf("; next %lu %s %s", static_cast<unsigned long>(p.next->date), PlanText::shortName(p.next->kind),
                    p.next->what);
    }
    std::printf("\n");
    if (expect != nullptr && std::strcmp(expect, stateName(p.state)) != 0) {
        std::printf("FAIL: expected %s\n", expect);
        return 1;
    }
    return 0;
}
