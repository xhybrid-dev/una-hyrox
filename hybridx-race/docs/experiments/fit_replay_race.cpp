// Replay a Roxzone-on race through the app's own ActivityWriter, RecordSpool
// and lapSeconds(), the way Service wires them, and write the .fit.
//
// fit_race_sample.cpp writes records straight to the writer, so it cannot show
// what Strava will be given now that the records carry a ramped distance. This
// does: one second at a time into the spool, a close() at every split with the
// distance of everything completed, laps batched afterwards.
//
// The segment times are the 31 laps of the 1 October 2026 test run
// (una-running-20261001-1248.fit), in whole seconds, so the new file can be
// compared with the one that went to Strava. With a split time a millisecond
// off a second boundary the real watch would differ by a second here or there;
// that is what lapSeconds() exists to keep consistent.
//
// Usage: ./fitreplay <out.fit> [days-ago]
// Build: see fit_replay_race.sh

#include "ActivityWriter.hpp"
#include "LapTiming.hpp"
#include "RaceData.hpp"
#include "RaceModel.hpp"
#include "RecordSpool.hpp"

#include "support/KernelTestDoubles.hpp"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

namespace fit = SDK::Fit;

namespace
{
// Seconds per segment, in plan order, from the 1 October test.
constexpr uint32_t kSeconds[31] = {95, 14, 235, 21, 89, 38, 96, 16, 97, 65, 218,
                                   12, 102, 45, 252, 26, 102, 27, 239, 24, 109, 16,
                                   171, 21, 112, 33, 248, 23, 106, 21, 264};
constexpr uint16_t kRunM = 400u;

struct Spooled {
    uint32_t timestamp;
    uint8_t  hr;
};

uint8_t hrAt(uint32_t t) { return static_cast<uint8_t>(110u + (t * 7u) % 70u); }
}  // namespace

int main(int argc, char **argv)
{
    const char *outName = (argc > 1) ? argv[1] : "replay.fit";
    const int daysAgo = (argc > 2) ? std::atoi(argv[2]) : 0;

    Race::SegmentDesc plan[Race::kMaxSegments] = {};
    const uint8_t n = Race::RaceModel::buildTemplate(Race::Format::Full, true, plan,
                                                     Race::kMaxSegments);
    if (n != 31u) {
        std::fprintf(stderr, "expected 31 segments, got %u\n", n);
        return 1;
    }

    uint32_t totalS = 0u;
    for (uint8_t i = 0u; i < n; ++i) totalS += kSeconds[i];

    SDK::TestSupport::KernelFixture fx;
    ActivityWriter writer(fx.kernel, ".");

    const std::time_t startUtc = std::time(nullptr) - static_cast<std::time_t>(totalS)
                                 - static_cast<std::time_t>(daysAgo) * 86400;

    ActivityWriter::AppInfo info {};
    info.timestamp = startUtc;
    info.appVersion = 0x00000100u;
    info.devID = "HybridX";
    info.appID = "8C345EF26E3350E7";
    writer.start(info);

    static char names[Race::kMaxSegments][Race::kMaxNameLen];
    ActivityWriter::WorkoutStepData steps[Race::kMaxSegments] = {};
    for (uint8_t i = 0u; i < n; ++i) {
        const uint16_t metres = Race::RaceModel::distanceM(plan[i], kRunM);
        Race::RaceModel::name(plan[i], names[i], sizeof(names[i]));
        steps[i].name = names[i];
        steps[i].intensity = fit::Intensity::Active;
        steps[i].durationType = metres ? fit::WktStepDuration::Distance : fit::WktStepDuration::Open;
        steps[i].durationValue = static_cast<uint32_t>(metres) * 100u;
    }
    writer.addWorkout("HYROX Full Race, 400 m runs", steps, n);

    // Records through the spool, exactly as Service::recordSecond / closeSpoolSegment.
    auto sink = [&writer](const Spooled &s, uint32_t cm) {
        ActivityWriter::RecordData r {};
        r.timestamp = static_cast<std::time_t>(s.timestamp);
        r.heartRate = static_cast<float>(s.hr);
        r.set(ActivityWriter::RecordData::Field::HEART_RATE);
        r.hrSource = 1u;
        r.hrOpticalBpm = s.hr;
        r.distanceCm = cm;
        writer.addRecord(r);
    };
    Race::RecordSpool<Spooled, 480> spool;

    uint32_t t = 0u;
    uint32_t doneCm = 0u;
    for (uint8_t i = 0u; i < n; ++i) {
        for (uint32_t k = 0u; k < kSeconds[i]; ++k, ++t) {
            spool.add({static_cast<uint32_t>(startUtc) + t, hrAt(t)}, sink);
        }
        doneCm += static_cast<uint32_t>(Race::RaceModel::distanceM(plan[i], kRunM)) * 100u;
        spool.close(doneCm, sink);
    }

    // Laps, batched, with the rounding the app now uses.
    uint32_t cursorMs = 0u;
    uint32_t distanceM = 0u;
    uint32_t hrSum = 0u, hrCount = 0u;
    uint8_t hrMaxAll = 0u;
    t = 0u;
    for (uint8_t i = 0u; i < n; ++i) {
        const Race::LapSeconds secs = Race::lapSeconds(cursorMs, kSeconds[i] * 1000u, 0u);
        uint32_t segSum = 0u;
        uint8_t segMax = 0u;
        for (uint32_t k = 0u; k < kSeconds[i]; ++k, ++t) {
            segSum += hrAt(t);
            if (hrAt(t) > segMax) segMax = hrAt(t);
        }
        hrSum += segSum;
        hrCount += kSeconds[i];
        if (segMax > hrMaxAll) hrMaxAll = segMax;

        ActivityWriter::LapData lap {};
        lap.timeStart = startUtc + secs.startSec;
        lap.timestamp = startUtc + secs.startSec + secs.elapsedSec;
        lap.duration = secs.activeSec;
        lap.elapsed = secs.elapsedSec;
        lap.hrAvg = static_cast<float>(segSum / kSeconds[i]);
        lap.hrMax = static_cast<float>(segMax);
        lap.segmentType = static_cast<uint8_t>(plan[i].type);
        lap.round = plan[i].round;
        lap.stationId = plan[i].stationId;
        lap.distanceM = Race::RaceModel::distanceM(plan[i], kRunM);
        lap.wktStepIndex = i;
        distanceM += lap.distanceM;
        writer.addLap(lap);
        cursorMs += kSeconds[i] * 1000u;
    }

    ActivityWriter::TrackData track {};
    track.timeStart = startUtc;
    track.timestamp = startUtc + totalS;
    track.duration = totalS;
    track.elapsed = totalS;
    track.hrAvg = static_cast<float>(hrSum / hrCount);
    track.hrMax = static_cast<float>(hrMaxAll);
    track.raceFormat = static_cast<uint8_t>(Race::Format::Full);
    track.roxzoneMode = 1u;
    track.completed = 1u;
    track.sport = static_cast<uint8_t>(fit::Sport::Running);
    track.subSport = static_cast<uint8_t>(fit::SubSport::Generic);
    track.distanceM = distanceM;
    track.runDistanceM = kRunM;
    if (!writer.stop(track)) {
        std::fprintf(stderr, "writer.stop() failed\n");
        return 1;
    }

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
    if (!out) return 1;
    std::fwrite(bytes.data(), 1, bytes.size(), out);
    std::fclose(out);
    std::printf("wrote %s: %zu bytes, %u laps, %u s, %u m\n", outName, bytes.size(),
                static_cast<unsigned>(n), totalS, distanceM);
    return 0;
}
