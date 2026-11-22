/**
 * @file    RunVo2Max.cpp
 * @brief   X Run's vo2max.json, read with every value range-checked (RunVo2Max.hpp).
 */

#include "RunVo2Max.hpp"

#include "SDK/JSON/JsonStreamReader.hpp"

namespace XTraining {

namespace RunVo2Max {

namespace {

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

bool read(const char* text, size_t len, Figure& out)
{
    out = Figure{};
    if (text == nullptr || len == 0 || len > kMaxBytes) {
        return false;
    }
    SDK::JsonStreamReader r(text, len);
    if (!r.validate()) {
        return false;
    }
    int64_t v = 0, runs = 0, x10 = 0, utc = 0;
    // Version 1 (X Run R6) and 2 (R6, with the trend) carry the same three keys used here. A
    // newer version may mean something else by them: it reads as none until this app learns it.
    if (!readInt(r, "v", 1, 2, v) || !readInt(r, "runs", 0, 255, runs) || runs == 0) {
        return false;
    }
    if (!readInt(r, "vo2maxX10", kMinX10, kMaxX10, x10) || !readInt(r, "utc", 1, 0xFFFFFFFFll, utc)) {
        return false;
    }
    out.x10  = static_cast<uint16_t>(x10);
    out.runs = static_cast<uint8_t>(runs);
    out.utc  = static_cast<uint32_t>(utc);
    return true;
}

}  // namespace RunVo2Max

}  // namespace XTraining
