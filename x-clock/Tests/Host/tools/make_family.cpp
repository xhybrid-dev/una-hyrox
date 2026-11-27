/**
 * make_family: writes a pretend watch's shared folder for the Clock Probe's simulator, with dates
 * that follow this machine's clock, so the ring always shows the current week.
 *
 *   make_family <SharedData dir> [full | noplan | empty | none | bad]
 *
 *   full    the whole family: sessions Monday to today, X Training's plan, X Run's VO2max
 *   noplan  as full, but X Training has no plan (plan 0)
 *   empty   no shared folder at all (a watch where no HybridX app has run)
 *   none    a shared folder with only the SDK's stride.json (the list opens, HybridX is missing)
 *   bad     every family file is there and none of them reads
 *
 * The files are written with the family's own writers (the Link codec, X Training's TodayFile),
 * so what the probe reads in the simulator is the real format.
 */

#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>

#include "PretendWatch.hpp"
#include "WeekView.hpp"

namespace fs = std::filesystem;
using HybridXLink::Sport;

namespace
{
void write(const fs::path& p, const std::string& text)
{
    fs::create_directories(p.parent_path());
    std::ofstream(p, std::ios::binary) << text;
}

int32_t localToday()
{
    const time_t now = std::time(nullptr);
    struct tm tm {};
    localtime_r(&now, &tm);
    return ClockProbe::daysFromCivil(tm.tm_year + 1900, static_cast<uint32_t>(tm.tm_mon + 1),
                                     static_cast<uint32_t>(tm.tm_mday));
}

/// X Training's file: a 10-week plan around this week, a row for each of the 14 days from Monday.
std::string todayFile(int32_t monday, int32_t today, bool plan)
{
    XTraining::TodayData d;
    if (!plan) {
        d.plan = 0;
    } else {
        const ClockProbe::CivilDate c = ClockProbe::civilFromDays(monday);
        const XTraining::Ymd first = XTraining::Date::make(c.year, c.month, c.day);
        d.plan  = 1;
        d.weeks = 10;
        d.start = XTraining::Date::addDays(first, -5 * 7);
        d.end   = XTraining::Date::addDays(first, 5 * 7 + 6);
        d.first = first;
        d.last  = XTraining::Date::addDays(first, 13);
        d.count = 14;
        struct Day { XTraining::Kind kind; const char* what; const char* pace; };
        static const Day kWeek[7] = {
            {XTraining::Kind::Easy, "35 min easy", "5:40/km"},
            {XTraining::Kind::Intervals, "4 x 4 min", "4:18/km"},
            {XTraining::Kind::Rest, "", ""},
            {XTraining::Kind::Tempo, "25 min tempo", "4:42/km"},
            {XTraining::Kind::Easy, "40 min easy", "5:50/km"},
            {XTraining::Kind::Long, "90 min long", "5:30/km"},
            {XTraining::Kind::Rest, "", ""},
        };
        for (int i = 0; i < 14; ++i) {
            XTraining::TodayDay& r = d.day[i];
            const Day& w = kWeek[i % 7];
            r.date    = XTraining::Date::addDays(first, i);
            r.kind    = w.kind;
            r.runWeek = static_cast<uint8_t>(6 + i / 7);
            r.status  = (monday + i < today && w.kind != XTraining::Kind::Rest) ? XTraining::TodayStatus::Done
                                                                               : XTraining::TodayStatus::ToCome;
            XTraining::TodayFile::setText(r.what, sizeof(r.what), w.what);
            XTraining::TodayFile::setText(r.pace, sizeof(r.pace), w.pace);
        }
    }
    char buf[XTraining::TodayFile::kMaxJsonBytes + 64];
    const size_t n = XTraining::TodayFile::write(d, buf, sizeof(buf));
    return std::string(buf, n);
}
} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "usage: make_family <SharedData dir> [full|noplan|empty|none|bad]\n");
        return 2;
    }
    const fs::path shared = argv[1];
    const std::string mode = argc > 2 ? argv[2] : "full";
    fs::remove_all(shared);
    if (mode == "empty") {
        return 0;
    }
    write(shared / "stride.json", "{\"stride\":[1,2,3]}");
    if (mode == "none") {
        return 0;
    }
    const fs::path hx = shared / "HybridX";
    fs::create_directories(hx);

    const int32_t today  = localToday();
    const int32_t monday = today - ClockProbe::weekdayOf(today);
    const int wd = static_cast<int>(ClockProbe::weekdayOf(today));

    if (mode == "bad") {
        for (const char* n : {"act_run", "act_race", "act_trail", "act_ride", "act_workout", "act_training"}) {
            write(hx / (std::string(n) + ".json"), "not a summary");
        }
        write(hx / "streak_now.json", "{\"v\":9}");
        write(hx / "training_today.json", "{oops");
        write(hx / "vo2max.json", "{\"v\":1}");
        return 0;
    }

    // Sessions Monday to today (never in the future), a mix that gives the ring all three grades.
    auto when = [&](int dayOffset, int hour, uint32_t minutes, Sport s, uint16_t load, uint32_t metres = 0) {
        return pretend::entry(monday + dayOffset, hour, minutes, s, load, metres);
    };
    std::vector<HybridXLink::Entry> run, race, trail, ride, workout, training;
    if (wd >= 0) { run.push_back(when(0, 7, 52, Sport::Run, 120, 9000)); ride.push_back(when(0, 17, 16, Sport::Ride, 39)); }
    if (wd >= 1) { trail.push_back(when(1, 9, 50, Sport::Run, 55)); workout.push_back(when(1, 18, 40, Sport::Strength, 0)); }
    if (wd >= 3) { race.push_back(when(3, 18, 64, Sport::Hybrid, 170)); }
    if (wd >= 4) { training.push_back(when(4, 6, 41, Sport::Run, 60, 7300)); }
    if (wd >= 5) { run.push_back(when(5, 9, 95, Sport::Run, 210, 18000)); }
    write(hx / "act_run.json", run.empty() ? "" : pretend::actText("HybridXRun", run));
    write(hx / "act_race.json", race.empty() ? "" : pretend::actText("HybridXRace", race));
    write(hx / "act_trail.json", trail.empty() ? "" : pretend::actText("HybridXTrail", trail));
    write(hx / "act_ride.json", ride.empty() ? "" : pretend::actText("HybridXRide", ride));
    write(hx / "act_workout.json", workout.empty() ? "" : pretend::actText("HybridXWorkout", workout));
    write(hx / "act_training.json", training.empty() ? "" : pretend::actText("HybridXTraining", training));
    for (const char* n : {"act_run", "act_race", "act_trail", "act_ride", "act_workout", "act_training"}) {
        const fs::path p = hx / (std::string(n) + ".json");
        if (fs::exists(p) && fs::file_size(p) == 0) {
            fs::remove(p);   // an app that has saved nothing yet leaves no file
        }
    }
    write(hx / "streak_now.json", pretend::streakNowText(monday, 4, 3, 8));
    write(hx / "training_today.json", todayFile(monday, today, mode != "noplan"));
    write(hx / "vo2max.json",
          "{\"v\":2,\"vo2maxX10\":523,\"runs\":9,\"utc\":1791000000,\"beforeX10\":519,"
          "\"trend\":[488,491,497,502,499,505,512,519,523]}");
    write(hx / "weighins.csv", "date,time,kg\n");   // never read by the probe; here to be seen in the list
    return 0;
}
