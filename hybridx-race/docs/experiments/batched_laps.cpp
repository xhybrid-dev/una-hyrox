// Phase 0 experiment: does a FIT file with ALL Lap messages written in one
// batch at save time -- after every Record, immediately before Session --
// decode cleanly?
//
// The HybridX Race brief (10.1) requires this shape, because a split can be
// undone and so a lap is not final until the race is saved. The SDK's own
// ActivityWriter instead streams each lap at lap time, so this ordering is
// NOT exercised anywhere in the SDK and has to be proven before Phase 3
// depends on it.
//
// Mirrors the real thing: 16 segments (Full race, Roxzone off), 1 Hz HR
// records, and the three segment developer fields from brief 10.1.

#include "SDK/Fit/FitWriter.hpp"
#include "SDK/Fit/FitProfile.hpp"
#include "support/KernelTestDoubles.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace fit = SDK::Fit;

// FIT timestamps count seconds from 1989-12-31T00:00:00Z.
static constexpr uint32_t kFitEpochOffset = 631065600u;
static uint32_t toFit(uint32_t unix) { return unix - kFitEpochOffset; }

// Local message types.
enum : uint8_t {
    L_FILE_ID = 0, L_DEV_ID = 1, L_FIELD_DESC = 2, L_EVENT = 3,
    L_RECORD = 4, L_LAP = 5, L_SESSION = 6, L_ACTIVITY = 7,
};

// Developer field numbers (brief 10.1).
enum : uint8_t {
    DF_SEGMENT_TYPE = 0, DF_ROUND = 1, DF_STATION_ID = 2,
    DF_RACE_FORMAT = 3, DF_ROXZONE_MODE = 4, DF_COMPLETED = 5,
};

struct Segment {
    const char* label;
    uint8_t type;       // 0 run, 1 rox in, 2 station, 3 rox out
    uint8_t round;      // 1..8
    uint8_t stationId;  // 1..8, 0 if not a station
    uint32_t durationS;
    uint8_t hrAvg;
    uint8_t hrMax;
};

static void writeFieldDescription(fit::FitWriter& w, uint8_t num, const char* name,
                                  const char* units, fit::BaseType type)
{
    const uint8_t nameLen = static_cast<uint8_t>(std::strlen(name) + 1);
    const uint8_t unitsLen = static_cast<uint8_t>((units ? std::strlen(units) : 0) + 1);
    w.defineMessage(L_FIELD_DESC, fit::mesgNum(fit::MesgNum::FieldDescription),
        {fit::field::FieldDescription::DeveloperDataIndex,
         fit::field::FieldDescription::FieldDefinitionNumber,
         fit::field::FieldDescription::FitBaseTypeId,
         {fit::field::FieldDescription::kFieldNameNum, fit::BaseType::String, nameLen},
         {fit::field::FieldDescription::kUnitsNum, fit::BaseType::String, unitsLen}});
    w.data(L_FIELD_DESC)
        .u8(0).u8(num).u8(static_cast<uint8_t>(type))
        .str(name, nameLen)
        .str(units ? units : "", unitsLen)
        .write();
}

int main()
{
    // A Full race, Roxzone off: RUN(r), STATION(r) for r = 1..8 => 16 segments.
    static const char* kStations[8] = {
        "SkiErg", "Sled Push", "Sled Pull", "Burpee Broad Jumps",
        "Row", "Farmers Carry", "Sandbag Lunges", "Wall Balls",
    };
    std::vector<Segment> segs;
    for (uint8_t r = 1; r <= 8; ++r) {
        segs.push_back({"RUN", 0, r, 0, static_cast<uint32_t>(300 + r), static_cast<uint8_t>(150 + r), static_cast<uint8_t>(165 + r)});
        segs.push_back({kStations[r - 1], 2, r, r, static_cast<uint32_t>(200 + r * 3), static_cast<uint8_t>(160 + r), static_cast<uint8_t>(175 + r)});
    }

    SDK::TestSupport::KernelFixture fx;
    auto file = fx.fileSystem.file("race.fit");
    if (!file || !file->open(true, true)) { std::fprintf(stderr, "open failed\n"); return 1; }

    fit::FitWriter w(*file);
    if (!w.begin(21u)) { std::fprintf(stderr, "begin failed\n"); return 1; }

    const uint32_t startUnix = 1790000000u;  // arbitrary fixed wall time

    // --- file_id -----------------------------------------------------------
    const uint8_t productNameLen = static_cast<uint8_t>(std::strlen(fit::kProductName) + 1);
    w.defineMessage(L_FILE_ID, fit::mesgNum(fit::MesgNum::FileId),
        {fit::field::FileId::Type, fit::field::FileId::Manufacturer,
         fit::field::FileId::Product, fit::field::FileId::SerialNumber,
         fit::field::FileId::TimeCreated,
         {fit::field::FileId::kProductNameNum, fit::BaseType::String, productNameLen}});
    w.data(L_FILE_ID)
        .u8(static_cast<uint8_t>(fit::File::Activity))
        .u16(static_cast<uint16_t>(fit::Manufacturer::Una))
        .u16(static_cast<uint16_t>(fit::Product::UnaWatch))
        .u32(0)
        .u32(toFit(startUnix))
        .str(fit::kProductName, productNameLen)
        .write();

    // --- developer_data_id -------------------------------------------------
    w.defineMessage(L_DEV_ID, fit::mesgNum(fit::MesgNum::DeveloperDataId),
        {fit::field::DeveloperDataId::ApplicationId,
         fit::field::DeveloperDataId::DeveloperDataIndex});
    {
        uint8_t appId[16] = {};
        std::strncpy(reinterpret_cast<char*>(appId), "HYBRIDXRACE00DEV", sizeof(appId));
        w.data(L_DEV_ID).bytes(appId, sizeof(appId)).u8(0).write();
    }

    // --- field descriptions ------------------------------------------------
    writeFieldDescription(w, DF_SEGMENT_TYPE, "segment_type", nullptr, fit::BaseType::UInt8);
    writeFieldDescription(w, DF_ROUND,        "round",        nullptr, fit::BaseType::UInt8);
    writeFieldDescription(w, DF_STATION_ID,   "station_id",   nullptr, fit::BaseType::UInt8);
    writeFieldDescription(w, DF_RACE_FORMAT,  "race_format",  nullptr, fit::BaseType::UInt8);
    writeFieldDescription(w, DF_ROXZONE_MODE, "roxzone_mode", nullptr, fit::BaseType::UInt8);
    writeFieldDescription(w, DF_COMPLETED,    "completed",    nullptr, fit::BaseType::UInt8);

    // --- definitions -------------------------------------------------------
    w.defineMessage(L_EVENT, fit::mesgNum(fit::MesgNum::Event),
        {fit::field::Event::Timestamp, fit::field::Event::EventField,
         fit::field::Event::EventType});
    w.defineMessage(L_RECORD, fit::mesgNum(fit::MesgNum::Record),
        {fit::field::Record::Timestamp, fit::field::Record::HeartRate});
    w.defineMessage(L_LAP, fit::mesgNum(fit::MesgNum::Lap),
        {fit::field::Lap::Timestamp, fit::field::Lap::StartTime,
         fit::field::Lap::TotalElapsedTime, fit::field::Lap::TotalTimerTime,
         fit::field::Lap::MessageIndex, fit::field::Lap::AvgHeartRate,
         fit::field::Lap::MaxHeartRate},
        {{DF_SEGMENT_TYPE, 1, 0}, {DF_ROUND, 1, 0}, {DF_STATION_ID, 1, 0}});
    w.defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
        {fit::field::Session::Timestamp, fit::field::Session::StartTime,
         fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
         fit::field::Session::MessageIndex, fit::field::Session::NumLaps,
         fit::field::Session::Sport, fit::field::Session::SubSport,
         fit::field::Session::AvgHeartRate, fit::field::Session::MaxHeartRate},
        {{DF_RACE_FORMAT, 1, 0}, {DF_ROXZONE_MODE, 1, 0}, {DF_COMPLETED, 1, 0}});
    w.defineMessage(L_ACTIVITY, fit::mesgNum(fit::MesgNum::Activity),
        {fit::field::Activity::Timestamp, fit::field::Activity::TotalTimerTime,
         fit::field::Activity::LocalTimestamp, fit::field::Activity::NumSessions});

    // --- timer start -------------------------------------------------------
    w.data(L_EVENT).u32(toFit(startUnix))
        .u8(static_cast<uint8_t>(fit::Event::Timer))
        .u8(static_cast<uint8_t>(fit::EventType::Start)).write();

    // --- 1 Hz records for the whole race -----------------------------------
    uint32_t totalS = 0;
    for (const Segment& s : segs) { totalS += s.durationS; }

    uint32_t recordCount = 0;
    for (uint32_t t = 0; t < totalS; ++t) {
        const uint8_t hr = static_cast<uint8_t>(150 + (t % 25));
        w.data(L_RECORD).u32(toFit(startUnix + t)).u8(hr).write();
        ++recordCount;
    }

    // --- THE POINT OF THE EXPERIMENT ---------------------------------------
    // All 16 laps, in chronological order, in one batch AFTER every record and
    // BEFORE the session. This is what undo-able splits force us to do.
    uint32_t cursor = 0;
    uint16_t lapIndex = 0;
    for (const Segment& s : segs) {
        const uint32_t segStart = startUnix + cursor;
        const uint32_t segEnd = segStart + s.durationS;
        w.data(L_LAP)
            .u32(toFit(segEnd))            // timestamp (segment end)
            .u32(toFit(segStart))          // start_time
            .u32(s.durationS * 1000u)      // total_elapsed_time, scale 1000
            .u32(s.durationS * 1000u)      // total_timer_time
            .u16(lapIndex)                 // message_index, 0-based sequential
            .u8(s.hrAvg)
            .u8(s.hrMax)
            .u8(s.type).u8(s.round).u8(s.stationId)
            .write();
        cursor += s.durationS;
        ++lapIndex;
    }

    // --- session, activity, finish -----------------------------------------
    w.data(L_SESSION)
        .u32(toFit(startUnix + totalS))
        .u32(toFit(startUnix))
        .u32(totalS * 1000u)
        .u32(totalS * 1000u)
        .u16(0)
        .u16(lapIndex)
        .u8(10)   // sport = training  (D2 candidate)
        .u8(0)    // sub_sport = generic
        .u8(158).u8(184)
        .u8(0).u8(0).u8(1)   // race_format=full, roxzone=off, completed=yes
        .write();
    w.data(L_ACTIVITY)
        .u32(toFit(startUnix + totalS))
        .u32(totalS * 1000u)
        .u32(toFit(startUnix + totalS))
        .u16(1)
        .write();

    const bool finished = w.finish();
    file->flush();
    file->close();

    if (!finished || !w.ok()) { std::fprintf(stderr, "finish failed\n"); return 1; }

    const std::string bytes = fx.fileSystem.readFile("race.fit");
    std::FILE* out = std::fopen("race.fit", "wb");
    if (!out) { std::fprintf(stderr, "cannot write race.fit\n"); return 1; }
    std::fwrite(bytes.data(), 1, bytes.size(), out);
    std::fclose(out);

    std::printf("wrote race.fit: %zu bytes, %u records, %u laps, %u s of race\n",
                bytes.size(), recordCount, lapIndex, totalS);
    return 0;
}
