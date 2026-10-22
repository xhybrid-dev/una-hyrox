// Write a sauna session FIT file THROUGH THE APP'S OWN ActivityWriter and
// SessionModel, over the SDK's kernel test doubles (an in-memory file system),
// so the .fit is what the watch writes, minus real sensor noise.
//
// Usage:  make_fit <out.fit> [scenario] [profile-name]
//   scenario  full     (default) sauna + plunge + rest, with a pause
//             sauna    just sauna (sauna <-> rest)
//             nohr     like full, but the watch never gives a heart rate
//             crash    records only, then the "app dies"; a new writer recovers it
//   profile-name  session.sport_profile_name; "-" for none. Default "Sauna".
//
// The numbers are invented to give a decoder something to tell apart (an HR
// that falls after each phase, a temperature that follows SimTemp). They say
// nothing about the real sensors.

#include "ActivityWriter.hpp"
#include "SessionFit.hpp"
#include "SessionModel.hpp"
#include "SimTemp.hpp"

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
    Sauna::Phase phase;     // what the person is doing for this stretch
    uint32_t     seconds;
    bool         pauseMid;  // pause for 20 s in the middle
};

uint16_t hrFor(Sauna::Phase p, uint32_t secIntoStep)
{
    switch (p) {
    case Sauna::Phase::Sauna:  return static_cast<uint16_t>(95 + secIntoStep / 8);          // climbing
    case Sauna::Phase::Plunge: return static_cast<uint16_t>(135 - secIntoStep / 3);         // dropping
    default:                   return static_cast<uint16_t>(110 - secIntoStep / 4);
    }
}

}  // namespace

int main(int argc, char **argv)
{
    const char *outName   = argc > 1 ? argv[1] : "sauna.fit";
    const std::string scenario = argc > 2 ? argv[2] : "full";
    std::string profile   = argc > 3 ? argv[3] : "Sauna";
    if (profile == "-") {
        profile.clear();
    }

    const bool justSauna = scenario == "sauna";
    const bool noHr      = scenario == "nohr";
    const bool crash     = scenario == "crash";

    Sauna::SessionConfig cfg;
    cfg.mode = justSauna ? Sauna::Mode::SaunaOnly : Sauna::Mode::SaunaPlunge;
    cfg.rest = true;

    // The script: what the person does, in order, as phase and length.
    std::vector<Step> steps;
    if (justSauna) {
        steps = {{Sauna::Phase::Sauna, 150, false}, {Sauna::Phase::Rest, 90, false},
                 {Sauna::Phase::Sauna, 150, false}};
    } else {
        steps = {{Sauna::Phase::Sauna, 150, true}, {Sauna::Phase::Plunge, 45, false},
                 {Sauna::Phase::Rest, 100, false}, {Sauna::Phase::Sauna, 120, false}};
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
    info.timestamp  = startUtc;
    info.appVersion = 0x00000100u;
    info.devID      = "HybridX";
    info.appID      = "400AE282F6C2C161";
    info.sportProfileName = profile;
    writer.start(info);

    static Sauna::SessionModel model;
    const uint32_t clockBase = 4000000000u;      // near the 32-bit wrap, on purpose
    model.start(clockBase, cfg);

    uint32_t t = 0;   // seconds since the start
    auto sample = [&](uint16_t hr, bool recordIt) {
        ++t;
        const uint32_t nowMs = clockBase + t * 1000u;
        const int32_t temp = Sauna::syntheticTempX100(t);
        if (recordIt) {
            ActivityWriter::RecordData r {};
            r.timestamp     = startUtc + static_cast<std::time_t>(t);
            r.heartRate     = noHr ? 0 : static_cast<uint8_t>(hr);
            r.watchTempX100 = temp;
            writer.addRecord(r);
        }
        model.addSample(nowMs, noHr ? 0 : hr, temp);
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
                    sample(hrFor(s.phase, i), false);     // recovery keeps feeding, no records
                }
                writer.resume(startUtc + static_cast<std::time_t>(t));
                model.resume(clockBase + t * 1000u);
            }
            sample(hrFor(s.phase, i), true);
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

        static Sauna::FitLap laps[Sauna::kMaxLaps];
        Sauna::FitSession fs;
        const uint8_t n = Sauna::buildFit(model, laps, Sauna::kMaxLaps, fs);
        for (uint8_t i = 0; i < n; ++i) {
            ActivityWriter::LapData lap {};
            lap.timeStart   = startUtc + static_cast<std::time_t>(laps[i].startSec);
            lap.timestamp   = startUtc + static_cast<std::time_t>(laps[i].startSec + laps[i].elapsedSec);
            lap.elapsed     = laps[i].elapsedSec;
            lap.duration    = laps[i].activeSec;
            lap.hrAvg       = laps[i].hrAvg;
            lap.hrMax       = laps[i].hrMax;
            lap.phase       = laps[i].phase;
            lap.round       = laps[i].round;
            lap.hasRecovery = laps[i].hasRecovery;
            lap.recoveryBpm = laps[i].recoveryBpm;
            writer.addLap(lap);
        }

        ActivityWriter::TrackData track {};
        track.timeStart   = startUtc;
        track.timestamp   = startUtc + static_cast<std::time_t>(fs.elapsedSec);
        track.elapsed     = fs.elapsedSec;
        track.duration    = fs.timerSec;
        track.hrAvg       = fs.hrAvg;
        track.hrMax       = fs.hrMax;
        track.mode        = fs.mode;
        track.restEnabled = fs.rest;
        track.completed   = 1;
        if (!writer.stop(track)) {
            std::fprintf(stderr, "writer.stop() failed\n");
            return 1;
        }
        std::printf("session: %u laps, elapsed %u s, timer %u s, hr avg %u max %u\n", n, fs.elapsedSec,
                    fs.timerSec, fs.hrAvg, fs.hrMax);
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
