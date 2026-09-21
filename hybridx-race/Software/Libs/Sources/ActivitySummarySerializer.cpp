/**
 ******************************************************************************
 * @file    ActivitySummarySerializer.cpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   Reads and writes the "Last race" summary as JSON (brief 10.2).
 ******************************************************************************
 *
 * Adapted from the SDK's Running/RunLVGL example, keeping its idiom: stream
 * straight into the file on save, read the whole file and query by dotted path
 * on load.
 *
 * The one rule worth restating (brief 14.3): a reader getter leaves its output
 * untouched when the key is missing and returns false. So every field is given
 * a deterministic value *before* the read, and an older file simply keeps the
 * defaults rather than picking up whatever was on the stack.
 *
 ******************************************************************************
 */

#include "ActivitySummarySerializer.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <string_view>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#define LOG_MODULE_PRX   "SummarySer"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

ActivitySummarySerializer::ActivitySummarySerializer(const SDK::Kernel &kernel,
                                                     const char *pathToFile)
        : mKernel(kernel)
        , mPath(pathToFile)
{
}

bool ActivitySummarySerializer::save(const ActivitySummary &summary)
{
    if (mPath == nullptr) {
        return false;
    }

    // Create the parent directory if the summary lives in one.
    const char *slash = std::strrchr(mPath, '/');
    if (slash != nullptr) {
        char dir[SDK::Interface::IFileSystem::skMaxPathLen] {};
        std::snprintf(dir, sizeof(dir), "%.*s", static_cast<int>(slash - mPath), mPath);
        if (!mKernel.fs.mkdir(dir)) {
            LOG_ERROR("Cannot create %s\n", dir);
            return false;
        }
    }

    auto file = mKernel.fs.file(mPath);
    if (!file || !file->open(true, true)) {
        LOG_ERROR("Cannot open %s\n", mPath);
        return false;
    }

    SDK::JsonStreamWriter writer(file.get());

    writer.startMap();

    writer.add("version", static_cast<uint32_t>(1));
    writer.add("format", static_cast<uint8_t>(summary.format));
    writer.add("roxzone", summary.roxzone);
    writer.add("completed", summary.completed);
    writer.add("start_utc", static_cast<uint32_t>(summary.startUtc));
    writer.add("total_ms", summary.totalMs);
    writer.add("runs_ms", summary.runsMs);
    writer.add("stations_ms", summary.stationsMs);
    writer.add("roxzone_ms", summary.roxzoneMs);
    writer.add("hr_avg", summary.hrAvg);
    writer.add("hr_max", summary.hrMax);
    writer.add("seg_count", summary.count);

    writer.startArray("segments");
    for (uint8_t i = 0u; i < summary.count && i < Race::kMaxSegments; ++i) {
        const SegmentSummary &s = summary.segments[i];
        writer.startMap();
        writer.add("t", s.type);
        writer.add("r", s.round);
        writer.add("s", s.stationId);
        writer.add("ms", s.durationMs);
        writer.add("ha", s.hrAvg);
        writer.add("hm", s.hrMax);
        writer.endMap();
    }
    writer.endArray();

    writer.endMap();

    file->flush();
    file->close();
    return true;
}

bool ActivitySummarySerializer::load(ActivitySummary &summary)
{
    summary = ActivitySummary {};  // deterministic defaults before any read

    if (mPath == nullptr) {
        return false;
    }

    auto file = mKernel.fs.file(mPath);
    if (!file || !file->exist() || !file->open()) {
        return false;
    }

    const size_t size = file->size();
    if (size == 0u) {
        file->close();
        return false;
    }

    char *buffer = new (std::nothrow) char[size];
    if (buffer == nullptr) {
        LOG_ERROR("Out of memory reading %s\n", mPath);
        file->close();
        return false;
    }

    size_t read = 0u;
    const bool readOk = file->read(buffer, size, read);
    file->close();

    if (!readOk || read == 0u) {
        delete[] buffer;
        return false;
    }

    SDK::JsonStreamReader reader(buffer, read);
    if (!reader.validate()) {
        LOG_ERROR("Summary JSON is invalid\n");
        delete[] buffer;
        return false;
    }

    uint8_t format = 0u;
    if (reader.get("format", format) && format <= static_cast<uint8_t>(Race::Format::HalfB)) {
        summary.format = static_cast<Race::Format>(format);
    }

    reader.get("roxzone", summary.roxzone);
    reader.get("completed", summary.completed);

    uint32_t startUtc = 0u;
    if (reader.get("start_utc", startUtc)) {
        summary.startUtc = static_cast<std::time_t>(startUtc);
    }

    reader.get("total_ms", summary.totalMs);
    reader.get("runs_ms", summary.runsMs);
    reader.get("stations_ms", summary.stationsMs);
    reader.get("roxzone_ms", summary.roxzoneMs);
    reader.get("hr_avg", summary.hrAvg);
    reader.get("hr_max", summary.hrMax);

    uint8_t count = 0u;
    reader.get("seg_count", count);
    if (count > Race::kMaxSegments) {
        // A corrupt count must not walk off the array: there is no MMU.
        LOG_WARNING("Summary claims %u segments, clamping\n", count);
        count = Race::kMaxSegments;
    }

    for (uint8_t i = 0u; i < count; ++i) {
        char query[32];
        SegmentSummary &s = summary.segments[i];

        std::snprintf(query, sizeof(query), "segments[%u].t", static_cast<unsigned>(i));
        reader.get(query, s.type);
        std::snprintf(query, sizeof(query), "segments[%u].r", static_cast<unsigned>(i));
        reader.get(query, s.round);
        std::snprintf(query, sizeof(query), "segments[%u].s", static_cast<unsigned>(i));
        reader.get(query, s.stationId);
        std::snprintf(query, sizeof(query), "segments[%u].ms", static_cast<unsigned>(i));
        reader.get(query, s.durationMs);
        std::snprintf(query, sizeof(query), "segments[%u].ha", static_cast<unsigned>(i));
        reader.get(query, s.hrAvg);
        std::snprintf(query, sizeof(query), "segments[%u].hm", static_cast<unsigned>(i));
        reader.get(query, s.hrMax);
    }

    summary.count = count;
    summary.valid = (count > 0u);

    delete[] buffer;
    return true;
}
