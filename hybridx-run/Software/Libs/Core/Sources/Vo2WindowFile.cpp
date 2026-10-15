/**
 ******************************************************************************
 * @file    Vo2WindowFile.cpp
 * @brief   One run's window records as a JSON file.
 ******************************************************************************
 */

#include "Vo2WindowFile.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/JSON/JsonStreamWriter.hpp"

namespace RunVo2
{

namespace
{

/// The SDK reader's integer getters do no type or range check (strtoul on
/// whatever is there), so read as int64 and bound it here.
bool readInt(const SDK::JsonStreamReader& r, const char* query, int64_t min, int64_t max, int64_t& out)
{
    int64_t v = 0;
    if (!r.get(query, v) || v < min || v > max) {
        return false;
    }
    out = v;
    return true;
}

}  // namespace

uint8_t windowFileSlotToWrite(const uint32_t* ids)
{
    uint8_t best = 0;
    for (uint8_t i = 0; i < kWindowFileSlots; ++i) {
        if (ids[i] == 0) {
            return i;
        }
        if (ids[i] < ids[best]) {
            best = i;
        }
    }
    return best;
}

size_t writeWindowFile(char* buf, size_t cap, uint32_t runId, uint8_t maxHr,
                       const WindowRecord* windows, uint16_t count)
{
    if (buf == nullptr || cap == 0) {
        return 0;
    }
    buf[0] = '\0';
    if (runId == 0 || (count > 0 && windows == nullptr)) {
        return 0;
    }
    if (count > kWindowFileMaxWindows) {
        count = kWindowFileMaxWindows;
    }

    SDK::JsonStreamWriter w(buf, cap);
    w.startMap();
    w.add("v", static_cast<uint8_t>(1));
    w.add("id", runId);
    w.add("max", maxHr);
    w.startArray("w");
    for (uint16_t i = 0; i < count; ++i) {
        const WindowRecord& r = windows[i];
        w.startArray();
        w.add(r.activeSec);
        w.add(r.goodSec);
        w.add(r.speedMMinX10);
        w.add(r.spreadX10);
        // Offset so the file holds no negative number: the SDK writer prints
        // signed values with "%ld", which is wrong where int32_t is an int (the
        // host tests), and the same file must read the same everywhere.
        w.add(static_cast<uint16_t>(r.gradeX10 + kGradeOffsetX10));
        w.add(r.steepSec);
        w.add(r.hrX10);
        w.add(r.hrMin);
        w.add(r.hrMax);
        w.endArray();
    }
    w.endArray();
    w.endMap();
    if (w.isError()) {
        buf[0] = '\0';
        return 0;
    }
    return std::strlen(buf);
}

bool WindowFileReader::open(WindowFileInfo& info)
{
    info = WindowFileInfo{};
    if (!mReader.validate()) {
        return false;
    }
    int64_t v = 0;
    if (!readInt(mReader, "v", 1, 1, v)) {
        return false;
    }
    int64_t id = 0, max = 0;
    if (!readInt(mReader, "id", 1, 0xFFFFFFFFll, id) ||
        !readInt(mReader, "max", 0, Config::kMaxHrBpm, max)) {
        return false;
    }
    size_t n = 0;
    if (!mReader.getArrayLength("w", n) || n > kWindowFileMaxWindows) {
        return false;
    }
    info.runId = static_cast<uint32_t>(id);
    info.maxHr = static_cast<uint8_t>(max);
    info.count = static_cast<uint16_t>(n);
    return true;
}

bool WindowFileReader::window(uint16_t i, WindowRecord& out) const
{
    out = WindowRecord{};
    if (i >= kWindowFileMaxWindows) {
        return false;
    }
    // [activeSec, goodSec, speed, spread, grade + offset, steep, hr, hrMin, hrMax]
    static constexpr int64_t kMin[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    static constexpr int64_t kMax[9] = {1000000, Config::kWindowSec, 6000, 6000, 2 * kGradeOffsetX10,
                                        Config::kWindowSec, Config::kMaxHrBpm * 10, 255, 255};
    int64_t f[9];
    for (unsigned k = 0; k < 9; ++k) {
        char q[24];
        std::snprintf(q, sizeof(q), "w[%u][%u]", static_cast<unsigned>(i), k);
        if (!readInt(mReader, q, kMin[k], kMax[k], f[k])) {
            return false;
        }
    }
    if (f[7] > f[8]) {   // hrMin above hrMax
        return false;
    }
    out.index        = static_cast<uint16_t>(i + 1);
    out.activeSec    = static_cast<uint32_t>(f[0]);
    out.goodSec      = static_cast<uint16_t>(f[1]);
    out.speedMMinX10 = static_cast<uint16_t>(f[2]);
    out.spreadX10    = static_cast<uint16_t>(f[3]);
    out.gradeX10     = static_cast<int16_t>(f[4] - kGradeOffsetX10);
    out.steepSec     = static_cast<uint8_t>(f[5]);
    out.hrX10        = static_cast<uint16_t>(f[6]);
    out.hrMin        = static_cast<uint8_t>(f[7]);
    out.hrMax        = static_cast<uint8_t>(f[8]);
    return true;
}

}  // namespace RunVo2
