/**
 ******************************************************************************
 * @file    ActivityWriter.cpp
 * @brief   Serializes a ride to a FIT file (see the header).
 ******************************************************************************
 */

#include "ActivityWriter.hpp"

#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include <cassert>
#include <cstring>

#define LOG_MODULE_PRX      "ActivityWriter"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace fit = SDK::Fit;

namespace {

// lap_trigger is field 24 of the lap message; manual is 0 and time is 1 in the
// public FIT profile (checked against the tables fitdecode generates from
// Garmin's SDK, as HybridX Race did); SDK/Fit/FitProfile.hpp declares neither.
constexpr fit::FitWriter::Field kLapTrigger{24, fit::BaseType::UInt8};
constexpr uint8_t kLapTriggerManual = 0u;
constexpr uint8_t kLapTriggerTime   = 1u;

/// sport_profile_name: session field 110, string (public FIT profile, as Race 5.28).
constexpr uint8_t kSportProfileNameNum = 110;

/// lap and session speeds are uint16 at scale 1000 (m/s): at most 65.534 m/s, below the invalid value.
uint16_t speedField(uint32_t mmPerS)
{
    return mmPerS > 65534u ? static_cast<uint16_t>(65534u) : static_cast<uint16_t>(mmPerS);
}

}  // namespace

using DevFieldDef = fit::FitWriter::DevField;

ActivityWriter::ActivityWriter(const SDK::Kernel& kernel, const char* pathToDir)
    : mKernel(kernel), mPath(pathToDir), mMarker(kernel.fs, pathToDir)
{
    assert(pathToDir != nullptr);
}

void ActivityWriter::start(const AppInfo& info)
{
    mLapCounter   = 0;
    mLastFlushUtc = info.timestamp;
    mOutdoor      = info.outdoor;
    mProfileName  = info.sportProfileName.substr(0, kProfileNameMax);

    if (!createAndOpenFile(info.timestamp)) {
        return;
    }

    mFit = std::make_unique<fit::FitWriter>(*mFile);
    const bool begun = mFit->begin(/*profileVersion=*/0);
    if (!begun) {
        LOG_ERROR("Failed to write FIT header\n");
    }

    // file_id
    const char* productName =
        info.productName.empty() ? fit::kProductName : info.productName.c_str();
    const uint8_t productNameLen = static_cast<uint8_t>(std::strlen(productName) + 1);
    mFit->defineMessage(L_FILE_ID, fit::mesgNum(fit::MesgNum::FileId),
        {fit::field::FileId::Type, fit::field::FileId::Manufacturer,
         fit::field::FileId::Product, fit::field::FileId::SerialNumber,
         fit::field::FileId::TimeCreated,
         {fit::field::FileId::kProductNameNum, fit::BaseType::String, productNameLen}});
    mFit->data(L_FILE_ID)
        .u8(static_cast<uint8_t>(fit::File::Activity))
        .u16(static_cast<uint16_t>(fit::Manufacturer::Una))
        .u16(static_cast<uint16_t>(fit::Product::UnaWatch))
        .u32(0)
        .u32(unixToFitTimestamp(info.timestamp))
        .str(productName, productNameLen)
        .write();

    // developer_data_id
    mFit->defineMessage(L_DEV_ID, fit::mesgNum(fit::MesgNum::DeveloperDataId),
        {fit::field::DeveloperDataId::ApplicationId,
         fit::field::DeveloperDataId::DeveloperDataIndex});
    {
        uint8_t appId[16] = {};
        std::strncpy(reinterpret_cast<char*>(appId), info.appID.c_str(), sizeof(appId));
        mFit->data(L_DEV_ID).bytes(appId, sizeof(appId)).u8(0).write();
    }

    // Developer field description (label and units survive any profile): the
    // lap's resting calories, as the SDK's Workout example names it.
    writeFieldDescription(DF_LAP_RESTING_CAL, "resting_calories", "kcal", fit::BaseType::UInt16);

    // event
    mFit->defineMessage(L_EVENT, fit::mesgNum(fit::MesgNum::Event),
        {fit::field::Event::Timestamp, fit::field::Event::EventField,
         fit::field::Event::EventType});

    const fit::FitWriter::Field profileName{kSportProfileNameNum, fit::BaseType::String,
                                            static_cast<uint8_t>(mProfileName.size() + 1u)};
    if (!mOutdoor) {
        // record: heart rate.
        mFit->defineMessage(L_RECORD, fit::mesgNum(fit::MesgNum::Record),
            {fit::field::Record::Timestamp, fit::field::Record::HeartRate});

        mFit->defineMessage(L_LAP, fit::mesgNum(fit::MesgNum::Lap),
            {fit::field::Lap::Timestamp, fit::field::Lap::StartTime,
             fit::field::Lap::TotalElapsedTime, fit::field::Lap::TotalTimerTime,
             fit::field::Lap::MessageIndex,
             fit::field::Lap::AvgHeartRate, fit::field::Lap::MaxHeartRate,
             fit::field::Lap::TotalCalories, kLapTrigger},
            {{DF_LAP_RESTING_CAL, 2, 0}});
        if (mProfileName.empty()) {
            mFit->defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
                {fit::field::Session::Timestamp, fit::field::Session::StartTime,
                 fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
                 fit::field::Session::MessageIndex, fit::field::Session::NumLaps,
                 fit::field::Session::Sport, fit::field::Session::SubSport,
                 fit::field::Session::AvgHeartRate, fit::field::Session::MaxHeartRate,
                 fit::field::Session::TotalCalories, fit::field::Session::MetabolicCalories});
        } else {
            mFit->defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
                {fit::field::Session::Timestamp, fit::field::Session::StartTime,
                 fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
                 fit::field::Session::MessageIndex, fit::field::Session::NumLaps,
                 fit::field::Session::Sport, fit::field::Session::SubSport,
                 fit::field::Session::AvgHeartRate, fit::field::Session::MaxHeartRate,
                 fit::field::Session::TotalCalories, fit::field::Session::MetabolicCalories,
                 profileName});
        }
    } else {
        // Outdoor records: with and without a position (the fix can come and go).
        mFit->defineMessage(L_RECORD_O, fit::mesgNum(fit::MesgNum::Record),
            {fit::field::Record::Timestamp, fit::field::Record::Distance,
             fit::field::Record::EnhancedAltitude, fit::field::Record::EnhancedSpeed,
             fit::field::Record::HeartRate});
        mFit->defineMessage(L_RECORD_OG, fit::mesgNum(fit::MesgNum::Record),
            {fit::field::Record::Timestamp,
             fit::field::Record::PositionLat, fit::field::Record::PositionLong,
             fit::field::Record::Distance,
             fit::field::Record::EnhancedAltitude, fit::field::Record::EnhancedSpeed,
             fit::field::Record::HeartRate});

        mFit->defineMessage(L_LAP, fit::mesgNum(fit::MesgNum::Lap),
            {fit::field::Lap::Timestamp, fit::field::Lap::StartTime,
             fit::field::Lap::TotalElapsedTime, fit::field::Lap::TotalTimerTime,
             fit::field::Lap::MessageIndex,
             fit::field::Lap::AvgHeartRate, fit::field::Lap::MaxHeartRate,
             fit::field::Lap::TotalCalories, kLapTrigger,
             fit::field::Lap::TotalDistance, fit::field::Lap::AvgSpeed,
             fit::field::Lap::MaxSpeed, fit::field::Lap::TotalAscent,
             fit::field::Lap::TotalDescent},
            {{DF_LAP_RESTING_CAL, 2, 0}});
        if (mProfileName.empty()) {
            mFit->defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
                {fit::field::Session::Timestamp, fit::field::Session::StartTime,
                 fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
                 fit::field::Session::MessageIndex, fit::field::Session::NumLaps,
                 fit::field::Session::Sport, fit::field::Session::SubSport,
                 fit::field::Session::AvgHeartRate, fit::field::Session::MaxHeartRate,
                 fit::field::Session::TotalCalories, fit::field::Session::MetabolicCalories,
                 fit::field::Session::TotalDistance, fit::field::Session::AvgSpeed,
                 fit::field::Session::MaxSpeed, fit::field::Session::TotalAscent,
                 fit::field::Session::TotalDescent});
        } else {
            mFit->defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
                {fit::field::Session::Timestamp, fit::field::Session::StartTime,
                 fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
                 fit::field::Session::MessageIndex, fit::field::Session::NumLaps,
                 fit::field::Session::Sport, fit::field::Session::SubSport,
                 fit::field::Session::AvgHeartRate, fit::field::Session::MaxHeartRate,
                 fit::field::Session::TotalCalories, fit::field::Session::MetabolicCalories,
                 fit::field::Session::TotalDistance, fit::field::Session::AvgSpeed,
                 fit::field::Session::MaxSpeed, fit::field::Session::TotalAscent,
                 fit::field::Session::TotalDescent, profileName});
        }
    }
    mFit->defineMessage(L_ACTIVITY, fit::mesgNum(fit::MesgNum::Activity),
        {fit::field::Activity::Timestamp, fit::field::Activity::TotalTimerTime,
         fit::field::Activity::LocalTimestamp, fit::field::Activity::NumSessions});

    addMessageEvent(info.timestamp, fit::EventType::Start);

    // Header + all definitions are on disk: flush and drop the recovery marker.
    // Only write the marker when begin() succeeded AND the flush is durable.
    if (begun && mFile->flush()) {
        mMarker.write(mFile->getPath(), static_cast<uint32_t>(mFile->getPosition()));
    }
}

void ActivityWriter::writeFieldDescription(uint8_t devFieldNum, const char* name,
                                           const char* units, fit::BaseType baseType)
{
    const uint8_t nameLen  = name ? static_cast<uint8_t>(std::strlen(name) + 1) : 1;
    const uint8_t unitsLen = units ? static_cast<uint8_t>(std::strlen(units) + 1) : 1;

    // Redefine the field_description slot to size the name/units strings exactly.
    mFit->defineMessage(L_FIELD_DESC, fit::mesgNum(fit::MesgNum::FieldDescription),
        {fit::field::FieldDescription::DeveloperDataIndex,
         fit::field::FieldDescription::FieldDefinitionNumber,
         fit::field::FieldDescription::FitBaseTypeId,
         {fit::field::FieldDescription::kFieldNameNum, fit::BaseType::String, nameLen},
         {fit::field::FieldDescription::kUnitsNum, fit::BaseType::String, unitsLen}});
    mFit->data(L_FIELD_DESC)
        .u8(0)
        .u8(devFieldNum)
        .u8(fit::baseTypeId(baseType))
        .str(name ? name : "", nameLen)
        .str(units ? units : "", unitsLen)
        .write();
}

void ActivityWriter::pause(std::time_t timestamp)
{
    if (mFit) {
        addMessageEvent(timestamp, fit::EventType::Stop);
    }
}

void ActivityWriter::resume(std::time_t timestamp)
{
    if (mFit) {
        addMessageEvent(timestamp, fit::EventType::Start);
    }
}

bool ActivityWriter::addRecord(const RecordData& record)
{
    if (!mFit) {
        return false;
    }

    const uint8_t hr = record.heartRate != 0
                           ? record.heartRate
                           : static_cast<uint8_t>(fit::baseTypeInvalid(fit::BaseType::UInt8));
    bool ok;
    if (!mOutdoor) {
        ok = mFit->data(L_RECORD).u32(unixToFitTimestamp(record.timestamp)).u8(hr).write();
    } else {
        const bool pos = record.hasPosition;
        fit::FitWriter::Data d = mFit->data(pos ? L_RECORD_OG : L_RECORD_O);
        d.u32(unixToFitTimestamp(record.timestamp));
        if (pos) {
            d.i32(degreesToSemicircles(record.latitude)).i32(degreesToSemicircles(record.longitude));
        }
        d.u32(record.distCm);
        // enhanced_altitude (5 * m + 500) and enhanced_speed (mm/s), invalid when unknown.
        const float alt5 = (record.altitudeM + 500.0f) * 5.0f;
        d.u32(record.hasAltitude && alt5 >= 0.0f && alt5 < 4294967040.0f
                  ? static_cast<uint32_t>(alt5)
                  : static_cast<uint32_t>(fit::baseTypeInvalid(fit::BaseType::UInt32)));
        d.u32(record.hasSpeed ? record.speedMmS
                              : static_cast<uint32_t>(fit::baseTypeInvalid(fit::BaseType::UInt32)));
        d.u8(hr);
        ok = d.write();
    }

    // Periodic durability flush: sync to eMMC and advance the marker to this
    // record boundary so a later crash recovers a record-complete file.
    if (record.timestamp - mLastFlushUtc >= skFlushIntervalSec) {
        if (mFile->flush()) {
            mMarker.update(static_cast<uint32_t>(mFile->getPosition()));
            mLastFlushUtc = record.timestamp;
        }
    }
    return ok;
}

void ActivityWriter::addLap(const LapData& lap)
{
    if (!mFit) {
        return;
    }
    fit::FitWriter::Data d = mFit->data(L_LAP);
    d.u32(unixToFitTimestamp(lap.timestamp))
        .u32(unixToFitTimestamp(lap.timeStart))
        .u32(static_cast<uint32_t>(lap.elapsed * 1000))
        .u32(static_cast<uint32_t>(lap.duration * 1000))
        .u16(mLapCounter);   // message_index: 0-based, incremented after this write
    for (uint8_t hr : {lap.hrAvg, lap.hrMax}) {
        d.u8(hr != 0 ? hr : static_cast<uint8_t>(fit::baseTypeInvalid(fit::BaseType::UInt8)));
    }
    d.u16(lap.kcal)
        .u8(lap.autoEnded ? kLapTriggerTime : kLapTriggerManual);
    if (mOutdoor) {
        d.u32(lap.distCm)
            .u16(speedField(lap.avgSpeedMmS))
            .u16(speedField(lap.maxSpeedMmS))
            .u16(lap.ascentM)
            .u16(lap.descentM);
    }
    // Developer field.
    d.u16(lap.restingKcal).write();
    mLapCounter++;
}

bool ActivityWriter::stop(const TrackData& track)
{
    if (!mFit) {
        return false;
    }

    // The timer started at start(); close the pair before the session.
    addMessageEvent(track.timestamp, fit::EventType::Stop);

    bool ok = mFit->ok();

    auto session = mFit->data(L_SESSION);
    session.u32(unixToFitTimestamp(track.timestamp))
        .u32(unixToFitTimestamp(track.timeStart))
        .u32(static_cast<uint32_t>(track.elapsed * 1000))
        .u32(static_cast<uint32_t>(track.duration * 1000))
        .u16(0)  // message_index
        .u16(mLapCounter)
        .u8(track.sport)
        .u8(track.subSport)
        .u8(track.hrAvg != 0 ? track.hrAvg : static_cast<uint8_t>(fit::baseTypeInvalid(fit::BaseType::UInt8)))
        .u8(track.hrMax != 0 ? track.hrMax : static_cast<uint8_t>(fit::baseTypeInvalid(fit::BaseType::UInt8)))
        .u16(track.kcal)
        .u16(track.metabolicKcal);
    if (mOutdoor) {
        session.u32(track.distCm)
            .u16(speedField(track.avgSpeedMmS))
            .u16(speedField(track.maxSpeedMmS))
            .u16(track.ascentM)
            .u16(track.descentM);
    }
    // The optional field comes last, as the definition orders it.
    if (!mProfileName.empty()) {
        session.str(mProfileName.c_str(), static_cast<uint8_t>(mProfileName.size() + 1u));
    }
    ok = session.write() && ok;

    ok = mFit->data(L_ACTIVITY)
        .u32(unixToFitTimestamp(track.timestamp))
        .u32(static_cast<uint32_t>(track.duration * 1000))
        .u32(unixToFitTimestamp(epochToLocal(track.timestamp)))
        .u16(1)
        .write() && ok;

    const bool finishOk = mFit->finish();
    if (!finishOk) {
        LOG_ERROR("Failed to finalize FIT file\n");
    }
    ok = finishOk && mFit->ok() && ok;
    mFit.reset();

    if (mFile) {
        ok = mFile->flush() && ok;
        ok = mFile->close() && ok;
    } else {
        ok = false;
    }

    // The .fit is durably finished on disk: drop the recovery marker so the
    // next boot does not treat this activity as interrupted.
    if (ok) {
        mMarker.remove();
    }

    // FIT durability IS the save-success contract: the kernel registers the .fit
    // when it closes. The .json summary is auxiliary and best-effort, and is
    // attempted only when the FIT is durable (Race's reasoning, kept).
    if (ok && !saveSummary(track)) {
        LOG_ERROR("Activity summary (.json) save failed; FIT is durable and registered\n");
    }
    return ok;
}

void ActivityWriter::discard()
{
    mFit.reset();
    mMarker.remove();
    if (!mFile) {
        return;
    }
    if (mFile->isOpen()) {
        mFile->close();
    }
    mFile->remove();
    mFile.reset();
}

void ActivityWriter::addMessageEvent(std::time_t t, fit::EventType type)
{
    mFit->data(L_EVENT)
        .u32(unixToFitTimestamp(t))
        .u8(static_cast<uint8_t>(fit::Event::Timer))
        .u8(static_cast<uint8_t>(type))
        .write();
}

bool ActivityWriter::recoverInterrupted()
{
    // All marker I/O + FitWriter::recover() orchestration lives in the shared
    // SDK::Fit::RecordingMarker. Recovery needs no sibling .json to register a
    // recovered activity (the kernel's activity registry tracks .fit files
    // only), so this is pure wiring.
    const auto result = mMarker.recover();
    if (result.recovered) {
        LOG_INFO("Recovery: finalized interrupted activity [%s]\n", result.path.c_str());
    }
    return result.recovered;
}

bool ActivityWriter::createAndOpenFile(std::time_t utc)
{
    char buff[256]{};
    std::tm localTime{};
#if WIN32
    localtime_s(&localTime, &utc);
#else
    localtime_r(&utc, &localTime);
#endif

    int len = snprintf(buff, sizeof(buff), "%s/%04u%02u/", mPath,
                       localTime.tm_year + 1900, localTime.tm_mon + 1);
    if (len <= 0 || !mKernel.fs.mkdir(buff)) {
        LOG_ERROR("Failed to create dir [%s]\n", buff);
        return false;
    }

    snprintf(&buff[len], sizeof(buff) - len, "activity_%04u%02u%02uT%02u%02u%02u.fit",
        localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
        localTime.tm_hour, localTime.tm_min, localTime.tm_sec);

    mFile = mKernel.fs.file(buff);
    if (!mFile || !mFile->open(true, true)) {
        LOG_ERROR("Failed to create file [%s]\n", buff);
        mFile.reset();
        return false;
    }

    return true;
}

bool ActivityWriter::saveSummary(const TrackData& track)
{
    if (!mFile) {
        return false;
    }
    char buff[256]{};
    size_t nameLen = strlen(mFile->getPath());
    snprintf(buff, sizeof(buff), "%.*s%s", static_cast<int>(nameLen - 3), mFile->getPath(), "json");

    mFile->setPath(buff);

    if (!mFile->open(true, true)) {
        LOG_ERROR("Failed to open activity summary [%s]\n", buff);
        mFile.reset();
        return false;
    }

    SDK::JsonStreamWriter writer(mFile.get());
    writer.startMap();
    writer.add("time_start", static_cast<uint32_t>(track.timeStart));
    writer.add("duration", static_cast<uint32_t>(track.duration));
    writer.add("hr_avg", track.hrAvg);
    writer.add("activity_type", "cycling");   // as the SDK's Cycling example
    writer.endMap();

    const bool ok = mFile->flush();
    return mFile->close() && ok;
}

std::time_t ActivityWriter::tm2epoch(const struct tm* tm)
{
    int y = tm->tm_year + 1900;
    int m = tm->tm_mon + 1;
    int d = tm->tm_mday;
    if (m <= 2) { y -= 1; m += 12; }
    int64_t  era = (y >= 0 ? y : y - 399) / 400;
    uint32_t yoe = (uint32_t)(y - era * 400);
    uint32_t doy = (153 * (m - 3) + 2) / 5 + d - 1;
    uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    int64_t days = era * 146097 + (int64_t)doe - 719468;
    int64_t secs = days * 86400 + tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
    return (std::time_t)secs;
}

std::time_t ActivityWriter::epochToLocal(std::time_t utc)
{
    std::tm localTime{};
#if WIN32
    localtime_s(&localTime, &utc);
#else
    localtime_r(&utc, &localTime);
#endif
    return tm2epoch(&localTime);
}

int32_t ActivityWriter::degreesToSemicircles(float degrees)
{
    return static_cast<int32_t>(degrees * (2147483648.0 / 180.0));
}

uint32_t ActivityWriter::unixToFitTimestamp(std::time_t unixTimestamp)
{
    const std::time_t FIT_EPOCH_OFFSET = 631065600;
    return static_cast<uint32_t>(unixTimestamp - FIT_EPOCH_OFFSET);
}
