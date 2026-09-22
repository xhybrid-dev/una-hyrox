// Write a realistic race FIT file THROUGH THE APP'S OWN ActivityWriter.
//
// Phase 0's batched_laps.cpp answered one question -- does a batch of laps
// written after the records decode? -- with a hand-written stand-in for the
// writer. That stand-in has now done its job and outlived its usefulness: when
// Jon uploaded its output to Garmin Connect (22 September 2026) the activity
// showed no distance, no pace, no moving time and unnamed laps, and nobody
// could tell from the file alone whether the app would do the same.
//
// This program removes the doubt. It drives the REAL ActivityWriter and the
// REAL RaceModel over the SDK's kernel test doubles, so the .fit it produces is
// byte-for-byte what the watch writes, minus the sensor noise.
//
// Usage:  ./fitsample <out.fit> [sport] [sub_sport] [distance-policy]
//         sport/sub_sport default to Training/Generic; values must be ones
//         SDK/Fit/FitProfile.hpp declares.
//         distance-policy 0 (default) credits every distance the format states,
//         which is what the app does; 1 credits the runs only, for comparing
//         what the two look like in Garmin and Strava before committing.
//         days-ago (default 0) moves the race back that many days.
//         run-metres (default 1000) shortens the runs, as a sim often is.
//
// The race always ENDS at the moment the program runs, so two files made in the
// same run differ in start time. That matters more than it sounds: every
// candidate up to 23 September 2026 carried a hard-coded start of
// 2026-09-21 14:13:20, so Garmin Connect saw one activity being re-uploaded
// rather than several to compare, and the sport it was first filed under stuck
// (NOTES.md 5.12).

#include "ActivityWriter.hpp"
#include "RaceData.hpp"
#include "RaceModel.hpp"

#include "support/KernelTestDoubles.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

namespace fit = SDK::Fit;

namespace
{

/// A plausible time for one segment, in seconds. Not pacing data: a shape that
/// makes the file readable, with runs slowing and stations drifting, so a
/// decoder can tell the laps apart.
uint32_t segmentSeconds(const Race::SegmentDesc &d)
{
    switch (d.type) {
    case Race::SegmentType::Run:     return 300u + d.round;
    case Race::SegmentType::RoxIn:
    case Race::SegmentType::RoxOut:  return 25u + d.round;
    case Race::SegmentType::Station:
    default:                         return 200u + d.round * 3u;
    }
}

}  // namespace

int main(int argc, char **argv)
{
    const char *outName = (argc > 1) ? argv[1] : "race.fit";
    const uint8_t sport = (argc > 2) ? static_cast<uint8_t>(std::atoi(argv[2]))
                                     : static_cast<uint8_t>(fit::Sport::Training);
    const uint8_t subSport = (argc > 3) ? static_cast<uint8_t>(std::atoi(argv[3]))
                                        : static_cast<uint8_t>(fit::SubSport::Generic);
    const bool runsOnly = (argc > 4) && (std::atoi(argv[4]) == 1);
    const int daysAgo = (argc > 5) ? std::atoi(argv[5]) : 0;
    const uint16_t runM = Race::clampRunDistanceM(
            (argc > 6) ? static_cast<uint16_t>(std::atoi(argv[6]))
                       : Race::kRunDistanceDefaultM);

    // The app always uses RaceModel::distanceM(); the policy switch exists only
    // so the two can be compared side by side in a consumer app.
    auto distanceOf = [runsOnly, runM](const Race::SegmentDesc &d) -> uint16_t {
        if (runsOnly && d.type != Race::SegmentType::Run) {
            return 0u;
        }
        return Race::RaceModel::distanceM(d, runM);
    };

    // A Full race with Roxzone off: sixteen segments.
    Race::SegmentDesc plan[Race::kMaxSegments] = {};
    const uint8_t n = Race::RaceModel::buildTemplate(Race::Format::Full, false, plan,
                                                     Race::kMaxSegments);
    if (n == 0u) {
        std::fprintf(stderr, "buildTemplate failed\n");
        return 1;
    }

    // Total race time, needed before the start time so the race can end "now".
    uint32_t totalS = 0u;
    for (uint8_t i = 0u; i < n; ++i) {
        totalS += segmentSeconds(plan[i]);
    }

    SDK::TestSupport::KernelFixture fx;
    ActivityWriter writer(fx.kernel, ".");

    const std::time_t startUtc = std::time(nullptr)
                                 - static_cast<std::time_t>(totalS)
                                 - static_cast<std::time_t>(daysAgo) * 86400;

    ActivityWriter::AppInfo info {};
    info.timestamp = startUtc;
    info.appVersion = 0x00000100u;
    info.devID = "HybridX";
    info.appID = "8C345EF26E3350E7";
    writer.start(info);

    // The workout, exactly as Service::emitRaceWorkout() builds it.
    static char names[Race::kMaxSegments][Race::kMaxNameLen];
    ActivityWriter::WorkoutStepData steps[Race::kMaxSegments] = {};
    for (uint8_t i = 0u; i < n; ++i) {
        const uint16_t metres = distanceOf(plan[i]);
        Race::RaceModel::name(plan[i], names[i], sizeof(names[i]));
        steps[i].name = names[i];
        steps[i].intensity = fit::Intensity::Active;
        if (metres > 0u) {
            steps[i].durationType = fit::WktStepDuration::Distance;
            steps[i].durationValue = static_cast<uint32_t>(metres) * 100u;
        } else {
            steps[i].durationType = fit::WktStepDuration::Open;
            steps[i].durationValue = 0u;
        }
    }
    char wktName[48];
    if (runM == Race::kRunDistanceDefaultM) {
        snprintf(wktName, sizeof(wktName), "HYROX Full Race");
    } else {
        snprintf(wktName, sizeof(wktName), "HYROX Full Race, %u m runs",
                 static_cast<unsigned>(runM));
    }
    writer.addWorkout(wktName, steps, n);

    // 1 Hz heart-rate records for the whole race.
    for (uint32_t t = 0u; t < totalS; ++t) {
        ActivityWriter::RecordData rec {};
        rec.timestamp = startUtc + static_cast<std::time_t>(t);
        rec.heartRate = static_cast<float>(150u + (t % 25u));
        rec.set(ActivityWriter::RecordData::Field::HEART_RATE);
        writer.addRecord(rec);
    }

    // Laps, in one batch after every record, as saveRace() writes them.
    uint32_t cursor = 0u;
    uint32_t distanceM = 0u;
    uint32_t hrSum = 0u;
    uint8_t hrMaxAll = 0u;
    for (uint8_t i = 0u; i < n; ++i) {
        const uint32_t secs = segmentSeconds(plan[i]);

        uint32_t segHrSum = 0u;
        uint8_t segHrMax = 0u;
        for (uint32_t t = cursor; t < cursor + secs; ++t) {
            const uint8_t hr = static_cast<uint8_t>(150u + (t % 25u));
            segHrSum += hr;
            if (hr > segHrMax) { segHrMax = hr; }
            if (hr > hrMaxAll) { hrMaxAll = hr; }
        }
        hrSum += segHrSum;

        ActivityWriter::LapData lap {};
        lap.timeStart = startUtc + static_cast<std::time_t>(cursor);
        lap.timestamp = startUtc + static_cast<std::time_t>(cursor + secs);
        lap.duration = static_cast<std::time_t>(secs);
        lap.elapsed = static_cast<std::time_t>(secs);
        lap.hrAvg = static_cast<float>(segHrSum / secs);
        lap.hrMax = static_cast<float>(segHrMax);
        lap.segmentType = static_cast<uint8_t>(plan[i].type);
        lap.round = plan[i].round;
        lap.stationId = plan[i].stationId;
        lap.distanceM = distanceOf(plan[i]);
        lap.wktStepIndex = i;
        distanceM += lap.distanceM;

        writer.addLap(lap);
        cursor += secs;
    }

    ActivityWriter::TrackData track {};
    track.timeStart = startUtc;
    track.timestamp = startUtc + static_cast<std::time_t>(totalS);
    track.duration = static_cast<std::time_t>(totalS);
    track.elapsed = static_cast<std::time_t>(totalS);
    track.hrAvg = static_cast<float>(hrSum / totalS);
    track.hrMax = static_cast<float>(hrMaxAll);
    track.raceFormat = static_cast<uint8_t>(Race::Format::Full);
    track.roxzoneMode = 0u;
    track.completed = 1u;
    track.sport = sport;
    track.subSport = subSport;
    track.distanceM = distanceM;
    track.runDistanceM = runM;

    if (!writer.stop(track)) {
        std::fprintf(stderr, "writer.stop() failed\n");
        return 1;
    }

    // ActivityWriter names the file itself, inside a dated folder. Find it in
    // the double's file system and copy it out under the name we were asked for.
    std::string found;
    for (const auto &entry : fx.fileSystem.files) {
        if (entry.first.size() > 4u &&
            entry.first.compare(entry.first.size() - 4u, 4u, ".fit") == 0) {
            found = entry.first;
            break;
        }
    }
    if (found.empty()) {
        std::fprintf(stderr, "no .fit was written\n");
        return 1;
    }

    const std::string bytes = fx.fileSystem.readFile(found.c_str());
    std::FILE *out = std::fopen(outName, "wb");
    if (!out) {
        std::fprintf(stderr, "cannot write %s\n", outName);
        return 1;
    }
    std::fwrite(bytes.data(), 1, bytes.size(), out);
    std::fclose(out);

    char when[32] = {};
    std::tm utc {};
    gmtime_r(&startUtc, &utc);
    std::strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%SZ", &utc);
    std::printf("wrote %s: %zu bytes, %u laps, %u s, %u m, sport=%u sub_sport=%u, "
                "starts %s\n",
                outName, bytes.size(), static_cast<unsigned>(n), totalS, distanceM,
                static_cast<unsigned>(sport), static_cast<unsigned>(subSport), when);
    return 0;
}
