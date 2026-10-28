/**
 ******************************************************************************
 * @file    Vo2TrailIngest.cpp
 * @brief   Run's side of the Trail link: judge a window file from Trail.
 ******************************************************************************
 */

#include "Vo2TrailIngest.hpp"

namespace RunVo2
{

IngestStatus judgeWindowFile(const char* text, size_t len, uint32_t markUtc,
                             Vo2Run& run, WindowFileInfo& info, bool keptOnly)
{
    info = WindowFileInfo{};
    if (text == nullptr || len == 0) {
        return IngestStatus::NotAFile;
    }
    WindowFileReader reader(text, len);
    if (!reader.open(info)) {
        info = WindowFileInfo{};
        return IngestStatus::NotAFile;
    }
    if (info.runId <= markUtc) {
        return IngestStatus::AlreadyCounted;
    }
    run.reset();
    run.setLagKnownOnly(keptOnly);
    for (uint16_t i = 0; i < info.count; ++i) {
        WindowRecord w;
        if (!reader.window(i, w)) {
            run.reset();
            return IngestStatus::Corrupt;
        }
        run.addWindow(w);
    }
    run.noteSustainedMax(info.maxHr);
    return IngestStatus::Judged;
}

int nextWindowFileSlot(const uint32_t* ids, uint8_t count, uint32_t markUtc)
{
    int      best   = -1;
    uint32_t bestId = 0;
    for (uint8_t i = 0; i < count; ++i) {
        const uint32_t id = ids[i];
        if (id != 0 && id > markUtc && (best < 0 || id < bestId)) {
            best   = i;
            bestId = id;
        }
    }
    return best;
}

}  // namespace RunVo2
