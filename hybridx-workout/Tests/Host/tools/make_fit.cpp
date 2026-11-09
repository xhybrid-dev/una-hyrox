// Write a workout FIT file THROUGH THE APP'S OWN ActivityWriter and
// WorkoutModel, over the SDK's kernel test doubles (an in-memory file system),
// so the .fit is what the watch writes, minus real sensor noise.
//
// Usage:  make_fit <out.fit> [scenario]
//   scenario  full     (default) four laps by the button, a 20 s pause in the first
//             autolap  a 1-minute auto lap over 150 s: laps of 60, 60 and 30 s
//             nohr     like full, but the watch never gives a heart rate
//             crash    records only, then the "app dies"; a new writer recovers it
//   and one per kind of workout (W2), run through the app's PlanRunner with a
//   tick a second and the button pressed at scripted seconds:
//             strength  3 sets, 30 s rests: sets of 40, 45 and 35 s; the second
//                       rest is cut to 10 s by the button
//             emom      3 minutes
//             amrap     a 2-minute cap; rounds at 40 and 85 s
//             tabata    8 x 20/10 (3:50)
//             custom    30 s work, 15 s rest, 3 rounds
//             circuit   2 rounds, 500 m runs: run 120 s, SkiErg 80 s, run 130 s,
//                       sled push 70 s
//   A plan pauses the workout at its end; the script waits 10 s, then ends it.
//
// The heart rates are invented, a steady value per lap so each lap sits in one
// zone (on the watch's default thresholds), at an 80 kg weight. They say nothing
// about the real sensors.

#include "ActivityWriter.hpp"
#include "PlanRunner.hpp"
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

/// A plan scenario: the plan, and the seconds (from the start) the button is pressed.
struct PlanScript {
    Workout::PlanConfig   plan;
    std::vector<uint32_t> presses;
};

bool planScript(const std::string &name, PlanScript &out)
{
    using Workout::SessionType;
    PlanScript p;
    if (name == "strength") {
        p.plan.type = SessionType::Strength;
        p.plan.sets = 3;
        p.plan.setRestSec = 30;
        p.presses = {40, 115, 125, 160};
    } else if (name == "emom") {
        p.plan.type = SessionType::Emom;
        p.plan.emomMin = 3;
        p.presses = {30};             // ignored: the timers ignore the button
    } else if (name == "amrap") {
        p.plan.type = SessionType::Amrap;
        p.plan.amrapMin = 2;
        p.presses = {40, 85};
    } else if (name == "tabata") {
        p.plan.type = SessionType::Tabata;
    } else if (name == "custom") {
        p.plan.type = SessionType::Custom;
        p.plan.workSec = 30;
        p.plan.restSec = 15;
        p.plan.rounds = 3;
    } else if (name == "circuit") {
        p.plan.type = SessionType::Circuit;
        p.plan.circuitRounds = 2;
        p.plan.runM = 500;
        p.presses = {120, 200, 330, 400};
    } else {
        return false;
    }
    out = p;
    return true;
}

}  // namespace

int main(int argc, char **argv)
{
    const char *outName        = argc > 1 ? argv[1] : "workout.fit";
    const std::string scenario = argc > 2 ? argv[2] : "full";

    const bool autoLap = scenario == "autolap";
    const bool noHr    = scenario == "nohr";
    const bool crash   = scenario == "crash";
    PlanScript script;
    const bool isPlan  = planScript(scenario, script);

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
    uint32_t totalS = isPlan ? 600u : 0u;   // a plan's length is the runner's: room for the longest
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
    info.appID            = "B9171EBAF7066A30";
    info.sportProfileName = Workout::fitSportOf(script.plan.type).profileName;
    writer.start(info);

    static Workout::WorkoutModel model;
    static Workout::PlanRunner runner;
    const uint32_t clockBase = 4000000000u;      // near the 32-bit wrap, on purpose
    runner.start(script.plan, model, clockBase, cfg);   // a plain plan for the W0 scenarios

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

    if (isPlan) {
        // A tick a second, as the service does: the record, the sample, the
        // runner; the button at the scripted seconds. Heart rate climbs 100 to
        // 170 and back, a step a minute, so the zones change.
        size_t next = 0;
        while (!runner.done() && t < 900) {
            const uint16_t hr = static_cast<uint16_t>(100u + 10u * ((t / 60u) % 8u));
            sample(hr, true);
            const uint32_t nowMs = clockBase + t * 1000u;
            if (runner.tick(model, nowMs) == Workout::Cue::Done) {
                writer.pause(startUtc + static_cast<std::time_t>(t));
                break;
            }
            if (next < script.presses.size() && script.presses[next] == t) {
                ++next;
                if (runner.press(model, nowMs) == Workout::Cue::Done) {
                    writer.pause(startUtc + static_cast<std::time_t>(t));
                    break;
                }
            }
        }
        t += 10;   // a while on the "Done" screen before End
    }

    bool first = true;
    for (const Step &s : (isPlan ? std::vector<Step>() : steps)) {
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
            lap.rest        = laps[i].kind == static_cast<uint8_t>(Workout::StepKind::Rest);
            writer.addLap(lap);
        }
        // As the service: a strength workout's laps are also its FIT sets.
        const Workout::SessionType type = runner.config().type;
        if (Workout::writesSets(type)) {
            for (uint8_t i = 0; i < n; ++i) {
                ActivityWriter::SetData set {};
                set.timeStart  = startUtc + static_cast<std::time_t>(laps[i].startSec);
                set.timestamp  = startUtc + static_cast<std::time_t>(laps[i].startSec + laps[i].elapsedSec);
                set.durationMs = laps[i].activeSec * 1000u;
                set.rest       = laps[i].kind == static_cast<uint8_t>(Workout::StepKind::Rest);
                writer.addSet(set);
            }
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
        const Workout::FitSport sport = Workout::fitSportOf(type);
        track.sport         = sport.sport;
        track.subSport      = sport.subSport;
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
