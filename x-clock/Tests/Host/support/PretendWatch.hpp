/**
 ******************************************************************************
 * @file    PretendWatch.hpp
 * @brief   A pretend watch for the Clock Probe's host tests: the family's files
 *          in ../SharedData/HybridX, made with the apps' own writers (the Link
 *          codec, X Training's TodayFile::write) so the fixtures are the real
 *          format, plus vo2max.json as Run's glance documents it.
 *
 * The probe's sandbox is /Apps/ClockProbe, so "../SharedData/..." is
 * /Apps/SharedData/... in the tree.
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_TEST_PRETEND_WATCH_HPP
#define CLOCK_PROBE_TEST_PRETEND_WATCH_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "ActivityLink.hpp"
#include "CivilDate.hpp"
#include "StreakNow.hpp"
#include "TodayFile.hpp"
#include "TreeFileSystem.hpp"

namespace pretend
{

inline const char* kSandbox = "/Apps/ClockProbe";
inline const char* kHx      = "/Apps/SharedData/HybridX/";

/// Thursday 26 November 2026, the day this probe was written, as a day number.
inline int32_t thursday() { return ClockProbe::daysFromCivil(2026, 11, 26); }

/// An entry that started on @p dayNumber (local) at @p hour, in UTC (offset 0 so local == UTC).
inline HybridXLink::Entry entry(int32_t dayNumber, int hour, uint32_t minutes, HybridXLink::Sport sport,
                                uint16_t load, uint32_t distanceM = 0)
{
    const uint32_t utc = static_cast<uint32_t>(dayNumber) * 86400u + static_cast<uint32_t>(hour) * 3600u;
    return HybridXLink::makeEntry(utc, 0, static_cast<int64_t>(minutes) * 60, static_cast<float>(distanceM), 140.0f,
                                  165.0f, sport, load);
}

/// An activity file's text, made by the link's own encoder.
inline std::string actText(const char* app, const std::vector<HybridXLink::Entry>& entries)
{
    HybridXLink::ActivityFile f;
    std::strncpy(f.app, app, sizeof(f.app) - 1);
    for (const auto& e : entries) {
        if (f.count < HybridXLink::kMaxEntries) {
            f.entries[f.count++] = e;
        }
    }
    char buf[HybridXLink::kMaxBytes + 64];
    const size_t n = HybridXLink::encode(f, buf, sizeof(buf));
    return std::string(buf, n);
}

/// streak_now.json for the week of @p weekStartDay, goal @p target, @p counted sessions.
inline std::string streakNowText(int32_t weekStartDay, uint8_t target, uint8_t counted, uint16_t streak)
{
    HybridXLink::StreakNow s;
    s.day       = weekStartDay + 3;
    s.weekStart = weekStartDay;
    s.target    = target;
    s.counted   = counted;
    s.daysMask  = 0x0B;
    s.streak    = streak;
    char buf[HybridXLink::kStreakNowMaxBytes + 64];
    const size_t n = HybridXLink::encodeStreakNow(s, buf, sizeof(buf));
    return std::string(buf, n);
}

/// training_today.json: a 10-week plan, today (yyyymmdd) a session "4 x 4 min" at "4:18/km".
inline std::string todayText(uint32_t todayYmd, XTraining::TodayStatus status = XTraining::TodayStatus::ToCome)
{
    XTraining::TodayData d;
    d.plan  = 1;
    d.weeks = 10;
    d.start = 20261012;
    d.end   = 20261220;
    d.first = 20261123;
    d.last  = 20261206;
    d.count = 3;
    d.day[0].date = todayYmd - 1;
    d.day[0].kind = XTraining::Kind::Rest;
    d.day[0].runWeek = 7;   // the reader wants 1 to 255 on every row, rest days included
    d.day[1].date = todayYmd;
    d.day[1].kind = XTraining::Kind::Intervals;
    d.day[1].status = status;
    d.day[1].runWeek = 7;
    XTraining::TodayFile::setText(d.day[1].what, sizeof(d.day[1].what), "4 x 4 min");
    XTraining::TodayFile::setText(d.day[1].pace, sizeof(d.day[1].pace), "4:18/km");
    d.day[2].date = todayYmd + 1;
    d.day[2].kind = XTraining::Kind::Rest;
    d.day[2].runWeek = 7;
    char buf[XTraining::TodayFile::kMaxJsonBytes + 64];
    const size_t n = XTraining::TodayFile::write(d, buf, sizeof(buf));
    return std::string(buf, n);
}

/// vo2max.json as Run's glance documents it (version 2).
inline std::string vo2Text()
{
    return "{\"v\":2,\"vo2maxX10\":523,\"runs\":5,\"utc\":1791000000,\"beforeX10\":519,"
           "\"trend\":[505,512,519,523]}";
}

/// A watch with the whole family: every activity app, Streak, X Training and X Run, plus the files
/// that are not ours (a weight log, the SDK's stride file, and a stranger's).
inline void addFamily(TreeFileSystem& fs, time_t stamp = 1795000000)
{
    const int32_t thu = thursday();
    const int32_t mon = thu - 3;
    using HybridXLink::Sport;
    fs.addFile(std::string(kHx) + "act_run.json",
               actText("HybridXRun", {entry(mon, 7, 52, Sport::Run, 120, 9000), entry(thu, 7, 30, Sport::Run, 60)}),
               stamp);
    fs.addFile(std::string(kHx) + "act_race.json", actText("HybridXRace", {entry(mon + 2, 18, 64, Sport::Hybrid, 170)}),
               stamp);
    fs.addFile(std::string(kHx) + "act_trail.json", actText("HybridXTrail", {entry(mon + 1, 9, 80, Sport::Run, 90)}),
               stamp);
    fs.addFile(std::string(kHx) + "act_ride.json", actText("HybridXRide", {entry(mon, 17, 16, Sport::Ride, 39)}),
               stamp);
    fs.addFile(std::string(kHx) + "act_workout.json", actText("HybridXWorkout", {entry(mon + 1, 12, 40, Sport::Strength, 0)}),
               stamp);
    fs.addFile(std::string(kHx) + "act_training.json", actText("HybridXTraining", {entry(mon + 3, 6, 50, Sport::Run, 80)}),
               stamp);
    fs.addFile(std::string(kHx) + "streak_now.json", streakNowText(mon, 4, 3, 8), stamp);
    fs.addFile(std::string(kHx) + "training_today.json", todayText(20261126), stamp);
    fs.addFile(std::string(kHx) + "vo2max.json", vo2Text(), stamp);
    fs.addFile(std::string(kHx) + "streak.json", "{\"streak\":{\"private\":\"state\"}}", stamp);
    fs.addFile(std::string(kHx) + "weighins.csv", "date,time,kg\n2026-11-26,07:30,82.40\n", stamp);
    fs.addFile("/Apps/SharedData/stride.json", "{\"stride\":[1,2,3]}", stamp);
    fs.addFile(std::string(kHx) + "someone_else.dat", "x", stamp);
}

} // namespace pretend

#endif // CLOCK_PROBE_TEST_PRETEND_WATCH_HPP
