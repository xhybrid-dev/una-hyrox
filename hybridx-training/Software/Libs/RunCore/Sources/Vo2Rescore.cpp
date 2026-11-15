/**
 ******************************************************************************
 * @file    Vo2Rescore.cpp
 * @brief   HybridX Run: scoring the kept runs again (see the header).
 ******************************************************************************
 */

#include "Vo2Rescore.hpp"

#include <cstdio>

#include "Vo2TrailIngest.hpp"

namespace RunVo2
{

uint32_t profileKey(const ProfileInput& in)
{
    // FNV-1a over the fields, so any change moves it; 0 is kept for "never scored".
    const uint8_t bytes[] = {
        kMethodVersion,
        static_cast<uint8_t>(in.birthYear & 0xFFu), static_cast<uint8_t>(in.birthYear >> 8),
        in.birthMonth, in.enteredMaxHr, in.enteredRestHr,
    };
    uint32_t h = 2166136261u;
    for (const uint8_t b : bytes) {
        h ^= b;
        h *= 16777619u;
    }
    return h == 0 ? 1u : h;
}

size_t runFilePath(char* out, size_t cap, uint32_t utc)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    const int n = std::snprintf(out, cap, "Runs/%lu.json", static_cast<unsigned long>(utc));
    if (n < 0 || static_cast<size_t>(n) >= cap) {
        out[0] = '\0';
        return 0;
    }
    return static_cast<size_t>(n);
}

RescoreOutcome rescoreRun(const char* text, size_t len, bool keptOnly, const Profile& profile,
                          Vo2Run& run, uint16_t& vo2x10, uint16_t& windows)
{
    vo2x10  = 0;
    windows = 0;
    WindowFileInfo info;
    // Mark 0: every file counts. A Corrupt file is as good as none: keep the run's figure.
    const IngestStatus st = judgeWindowFile(text, len, 0, run, info, keptOnly);
    if (st != IngestStatus::Judged) {
        run.reset();
        return RescoreOutcome::NoFile;
    }
    const RunResult r = run.estimate(profile);
    run.reset();
    if (r.status != RunStatus::Ok) {
        return RescoreOutcome::NoFigure;
    }
    vo2x10  = r.vo2x10;
    windows = r.windowsUsed;
    return RescoreOutcome::Scored;
}

}  // namespace RunVo2
