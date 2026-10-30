// Write a ride FIT file THROUGH THE APP'S OWN ActivityWriter and
// RideModel, over the SDK's kernel test doubles (an in-memory file system),
// so the .fit is what the watch writes, minus real sensor noise.
//
// Usage:  make_fit <out.fit> [scenario]
//   scenario  full     (default) four laps by the button, a 20 s pause in the first (indoor)
//             outdoor  like full, on GPS: 7 m/s, climbing in lap 1 and descending in lap 3, with
//                      no position for the first 30 s (a fix not yet found)
//             autolap  a 1-minute auto lap over 150 s: laps of 60, 60 and 30 s
//             nohr     like full, but the watch never gives a heart rate
//             crash    records only, then the "app dies"; a new writer recovers it
//
// The heart rates are invented, a steady value per lap so each lap sits in one
// zone (on the watch's default thresholds), at an 80 kg weight. They say nothing
// about the real sensors.

#include "ActivityWriter.hpp"
#include "RideFit.hpp"
#include "RideModel.hpp"

#include "support/KernelTestDoubles.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

namespace {

struct Step {
    uint16_t hr;        // the heart rate for this stretch
    uint32_t seconds;
    bool     pauseMid;  // pause for 20 s in the middle
};

}  // namespace

int main(int argc, char **argv)
{
    const char *outName        = argc > 1 ? argv[1] : "ride.fit";
    const std::string scenario = argc > 2 ? argv[2] : "full";

    const bool autoLap = scenario == "autolap";
    const bool noHr    = scenario == "nohr";
    const bool crash   = scenario == "crash";
    const bool outdoor = scenario == "outdoor";

    Ride::RideConfig cfg;
    cfg.weightG   = 80000;
    cfg.outdoor   = outdoor;
    cfg.autoLapMs = autoLap ? 60000u : 0u;

    // The script: each stretch's heart rate and length; a new stretch is a lap
    // by the button (not in the auto-lap scenario, which is one stretch).
    std::vector<Step> steps;
    if (autoLap) {
        steps = {{120, 150, false}};
    } else {
        steps = {{140, 150, true}, {160, 45, false}, {100, 100, false}, {120, 120, false}};
    }
    uint32_t totalS = 0;
    for (const Step &s : steps) {
        totalS += s.seconds + (s.pauseMid ? 20u : 0u);
    }

    SDK::TestSupport::KernelFixture fx;
    std::unique_ptr<ActivityWriter> writerPtr(new ActivityWriter(fx.kernel, "Activity"));
    ActivityWriter &writer = *writerPtr;

    const std::time_t startUtc = std::time(nullptr) - static_cast<std::time_t>(totalS) - 60;

    ActivityWriter::AppInfo info {};
    info.timestamp        = startUtc;
    info.appVersion       = 0x00000100u;
    info.devID            = "HybridX";
    info.appID            = "250009C2B1049597";
    info.sportProfileName = "Ride";
    info.outdoor          = outdoor;
    writer.start(info);

    static Ride::RideModel model;
    const uint32_t clockBase = 4000000000u;      // near the 32-bit wrap, on purpose
    model.start(clockBase, cfg);

    uint32_t t = 0;   // seconds since the start
    int autoLaps = 0;
    uint32_t recorded = 0;   // recorded seconds so far
    auto sample = [&](uint16_t hr, bool recordIt, int stretch) {
        ++t;
        const uint32_t nowMs = clockBase + t * 1000u;
        Ride::Motion m;
        if (outdoor && recordIt) {
            // 7 m/s; a metre climbed every 10th second of lap 1, 50 cm dropped every 10th of lap 3.
            m.distCm    = 700;
            m.speedMmS  = 7000;
            m.ascentCm  = (stretch == 0 && recorded % 10 == 0) ? 100 : 0;
            m.descentCm = (stretch == 2 && recorded % 10 == 0) ? 50 : 0;
        }
        const bool autoLapped = model.addSample(nowMs, noHr ? 0 : hr, m);
        autoLaps += autoLapped ? 1 : 0;
        if (recordIt) {
            ActivityWriter::RecordData r {};
            r.timestamp = startUtc + static_cast<std::time_t>(t);
            r.heartRate = noHr ? 0 : static_cast<uint8_t>(hr);
            if (outdoor) {
                r.hasPosition = recorded >= 30;                       // no fix for the first 30 s
                r.latitude    = 51.5f + 0.00006f * static_cast<float>(recorded);   // heading north
                r.longitude   = -0.12f;
                r.hasAltitude = true;
                r.altitudeM   = 40.0f + static_cast<float>(model.ascentCm()) / 100.0f
                                      - static_cast<float>(model.descentCm()) / 100.0f;
                r.hasSpeed    = true;
                r.speedMmS    = 7000;
                r.distCm      = model.distCm();
            }
            writer.addRecord(r);
            ++recorded;
        }
    };

    bool first = true;
    int stretch = -1;
    for (const Step &s : steps) {
        ++stretch;
        if (!first) {
            model.lap(clockBase + t * 1000u);
        }
        first = false;
        for (uint32_t i = 0; i < s.seconds; ++i) {
            if (s.pauseMid && i == s.seconds / 2) {
                writer.pause(startUtc + static_cast<std::time_t>(t));
                model.pause(clockBase + t * 1000u);
                for (int k = 0; k < 20; ++k) {
                    sample(s.hr, false, stretch);     // no records, and the model ignores them
                }
                writer.resume(startUtc + static_cast<std::time_t>(t));
                model.resume(clockBase + t * 1000u);
            }
            sample(s.hr, true, stretch);
        }
    }

    if (crash) {
        // The app dies here: no laps, no session, no stop(). A new writer on
        // the same file system, as the next boot, finishes the file.
        std::printf("crash: %u records written, recovering...\n", t);
        writerPtr.reset();                                     // abandoned, never finished
        writerPtr.reset(new ActivityWriter(fx.kernel, "Activity"));
        if (!writerPtr->recoverInterrupted()) {
            std::fprintf(stderr, "recoverInterrupted() recovered nothing\n");
            return 1;
        }
    } else {
        model.stop(clockBase + t * 1000u);

        static Ride::FitLap laps[Ride::kMaxLaps];
        Ride::FitSession fs;
        const uint8_t n = Ride::buildFit(model, laps, Ride::kMaxLaps, fs);
        for (uint8_t i = 0; i < n; ++i) {
            ActivityWriter::LapData lap {};
            lap.timeStart   = startUtc + static_cast<std::time_t>(laps[i].startSec);
            lap.timestamp   = startUtc + static_cast<std::time_t>(laps[i].startSec + laps[i].elapsedSec);
            lap.elapsed     = laps[i].elapsedSec;
            lap.duration    = laps[i].activeSec;
            lap.hrAvg       = laps[i].hrAvg;
            lap.hrMax       = laps[i].hrMax;
            lap.kcal        = laps[i].kcal;
            lap.restingKcal = laps[i].restingKcal;
            lap.autoEnded   = laps[i].autoEnded;
            lap.distCm      = laps[i].distCm;
            lap.avgSpeedMmS = laps[i].avgSpeedMmS;
            lap.maxSpeedMmS = laps[i].maxSpeedMmS;
            lap.ascentM     = laps[i].ascentM;
            lap.descentM    = laps[i].descentM;
            writer.addLap(lap);
        }

        ActivityWriter::TrackData track {};
        track.timeStart     = startUtc;
        track.timestamp     = startUtc + static_cast<std::time_t>(fs.elapsedSec);
        track.elapsed       = fs.elapsedSec;
        track.duration      = fs.timerSec;
        track.hrAvg         = fs.hrAvg;
        track.hrMax         = fs.hrMax;
        track.kcal          = fs.kcal;
        track.metabolicKcal = fs.restingKcal;
        track.sport         = static_cast<uint8_t>(SDK::Fit::Sport::Cycling);
        track.subSport      = static_cast<uint8_t>(outdoor ? SDK::Fit::SubSport::Generic
                                                           : SDK::Fit::SubSport::IndoorCycling);
        track.distCm        = fs.distCm;
        track.avgSpeedMmS   = fs.avgSpeedMmS;
        track.maxSpeedMmS   = fs.maxSpeedMmS;
        track.ascentM       = fs.ascentM;
        track.descentM      = fs.descentM;
        if (!writer.stop(track)) {
            std::fprintf(stderr, "writer.stop() failed\n");
            return 1;
        }
        std::printf("session: %u laps (%d auto), elapsed %u s, timer %u s, hr avg %u max %u, %u kcal (%u resting)\n",
                    n, autoLaps, fs.elapsedSec, fs.timerSec, fs.hrAvg, fs.hrMax, fs.kcal, fs.restingKcal);
        if (outdoor) {
            std::printf("outdoor: %lu cm, avg %lu max %lu mm/s, +%u m -%u m\n", static_cast<unsigned long>(fs.distCm),
                        static_cast<unsigned long>(fs.avgSpeedMmS), static_cast<unsigned long>(fs.maxSpeedMmS),
                        fs.ascentM, fs.descentM);
        }
    }

    std::string found;
    for (const auto &entry : fx.fileSystem.files) {
        if (entry.first.size() > 4u && entry.first.compare(entry.first.size() - 4u, 4u, ".fit") == 0) {
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
    std::printf("wrote %s (%zu bytes) from %s\n", outName, bytes.size(), found.c_str());
    return 0;
}
