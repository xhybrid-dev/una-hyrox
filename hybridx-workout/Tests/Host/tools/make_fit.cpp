// Write a workout FIT file THROUGH THE APP'S OWN ActivityWriter and
// WorkoutModel, over the SDK's kernel test doubles (an in-memory file system),
// so the .fit is what the watch writes, minus real sensor noise.
//
// Usage:  make_fit <out.fit> [scenario]
//   scenario  full     (default) four laps by the button, a 20 s pause in the first
//             autolap  a 1-minute auto lap over 150 s: laps of 60, 60 and 30 s
//             nohr     like full, but the watch never gives a heart rate
//             crash    records only, then the "app dies"; a new writer recovers it
//
// The heart rates are invented, a steady value per lap so each lap sits in one
// zone (on the watch's default thresholds), at an 80 kg weight. They say nothing
// about the real sensors.

#include "ActivityWriter.hpp"
#include "WorkoutFit.hpp"
#include "WorkoutModel.hpp"

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
    const char *outName        = argc > 1 ? argv[1] : "workout.fit";
    const std::string scenario = argc > 2 ? argv[2] : "full";

    const bool autoLap = scenario == "autolap";
    const bool noHr    = scenario == "nohr";
    const bool crash   = scenario == "crash";

    Workout::WorkoutConfig cfg;
    cfg.weightG   = 80000;
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
    info.appID            = "A294992CFC56DBE9";
    info.sportProfileName = "Workout";
    writer.start(info);

    static Workout::WorkoutModel model;
    const uint32_t clockBase = 4000000000u;      // near the 32-bit wrap, on purpose
    model.start(clockBase, cfg);

    uint32_t t = 0;   // seconds since the start
    int autoLaps = 0;
    auto sample = [&](uint16_t hr, bool recordIt) {
        ++t;
        const uint32_t nowMs = clockBase + t * 1000u;
        if (recordIt) {
            ActivityWriter::RecordData r {};
            r.timestamp = startUtc + static_cast<std::time_t>(t);
            r.heartRate = noHr ? 0 : static_cast<uint8_t>(hr);
            writer.addRecord(r);
        }
        autoLaps += model.addSample(nowMs, noHr ? 0 : hr) ? 1 : 0;
    };

    bool first = true;
    for (const Step &s : steps) {
        if (!first) {
            model.lap(clockBase + t * 1000u);
        }
        first = false;
        for (uint32_t i = 0; i < s.seconds; ++i) {
            if (s.pauseMid && i == s.seconds / 2) {
                writer.pause(startUtc + static_cast<std::time_t>(t));
                model.pause(clockBase + t * 1000u);
                for (int k = 0; k < 20; ++k) {
                    sample(s.hr, false);     // no records, and the model ignores them
                }
                writer.resume(startUtc + static_cast<std::time_t>(t));
                model.resume(clockBase + t * 1000u);
            }
            sample(s.hr, true);
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

        static Workout::FitLap laps[Workout::kMaxLaps];
        Workout::FitSession fs;
        const uint8_t n = Workout::buildFit(model, laps, Workout::kMaxLaps, fs);
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
        if (!writer.stop(track)) {
            std::fprintf(stderr, "writer.stop() failed\n");
            return 1;
        }
        std::printf("session: %u laps (%d auto), elapsed %u s, timer %u s, hr avg %u max %u, %u kcal (%u resting)\n",
                    n, autoLaps, fs.elapsedSec, fs.timerSec, fs.hrAvg, fs.hrMax, fs.kcal, fs.restingKcal);
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
