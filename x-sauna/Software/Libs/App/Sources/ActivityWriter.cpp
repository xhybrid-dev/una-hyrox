/**
 ******************************************************************************
 * @file    ActivityWriter.cpp
 * @brief   Serializes a sauna session to a FIT file (see the header).
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

// lap_trigger is field 24 of the lap message and manual is 0 in the public FIT
// profile (checked against the tables fitdecode generates from Garmin's SDK, as
// HybridX Race did); SDK/Fit/FitProfile.hpp declares neither. Every lap here is
// a button press, and saying so stops a consumer treating them as automatic.
constexpr fit::FitWriter::Field kLapTrigger{24, fit::BaseType::UInt8};
constexpr uint8_t kLapTriggerManual = 0u;

/// sport_profile_name: session field 110, string (public FIT profile, as Race 5.28).
constexpr uint8_t kSportProfileNameNum = 110;

/// The watch's temperature is written when there is one.
bool tempPresent(int32_t x100)
{
    return x100 != INT32_MIN;
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

    // Developer field descriptions (label and units survive any profile). No
    // unit is claimed for the temperature: it is the sensor's own number.
    writeFieldDescription(DF_WATCH_TEMP, "watch_temp", nullptr, fit::BaseType::Float32);
    writeFieldDescription(DF_PHASE, "phase", nullptr, fit::BaseType::UInt8);
    writeFieldDescription(DF_ROUND, "round", nullptr, fit::BaseType::UInt8);
    writeFieldDescription(DF_HR_RECOVERY, "hr_recovery_bpm", "bpm", fit::BaseType::SInt16);
    writeFieldDescription(DF_MODE, "mode", nullptr, fit::BaseType::UInt8);
    writeFieldDescription(DF_REST_ENABLED, "rest_enabled", nullptr, fit::BaseType::UInt8);
    writeFieldDescription(DF_COMPLETED, "completed", nullptr, fit::BaseType::UInt8);

    // event
    mFit->defineMessage(L_EVENT, fit::mesgNum(fit::MesgNum::Event),
        {fit::field::Event::Timestamp, fit::field::Event::EventField,
         fit::field::Event::EventType});

    // record: heart rate, and the watch's temperature as a developer field.
    mFit->defineMessage(L_RECORD, fit::mesgNum(fit::MesgNum::Record),
        {fit::field::Record::Timestamp, fit::field::Record::HeartRate},
        {{DF_WATCH_TEMP, 4, 0}});

    // lap / session / activity
    mFit->defineMessage(L_LAP, fit::mesgNum(fit::MesgNum::Lap),
        {fit::field::Lap::Timestamp, fit::field::Lap::StartTime,
         fit::field::Lap::TotalElapsedTime, fit::field::Lap::TotalTimerTime,
         fit::field::Lap::MessageIndex,
         fit::field::Lap::AvgHeartRate, fit::field::Lap::MaxHeartRate,
         kLapTrigger},
        {{DF_PHASE, 1, 0}, {DF_ROUND, 1, 0}, {DF_HR_RECOVERY, 2, 0}});
    if (mProfileName.empty()) {
        mFit->defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
            {fit::field::Session::Timestamp, fit::field::Session::StartTime,
             fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
             fit::field::Session::MessageIndex, fit::field::Session::NumLaps,
             fit::field::Session::Sport, fit::field::Session::SubSport,
             fit::field::Session::AvgHeartRate, fit::field::Session::MaxHeartRate},
            {{DF_MODE, 1, 0}, {DF_REST_ENABLED, 1, 0}, {DF_COMPLETED, 1, 0}});
    } else {
        mFit->defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
            {fit::field::Session::Timestamp, fit::field::Session::StartTime,
             fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
             fit::field::Session::MessageIndex, fit::field::Session::NumLaps,
             fit::field::Session::Sport, fit::field::Session::SubSport,
             fit::field::Session::AvgHeartRate, fit::field::Session::MaxHeartRate,
             {kSportProfileNameNum, fit::BaseType::String,
              static_cast<uint8_t>(mProfileName.size() + 1u)}},
            {{DF_MODE, 1, 0}, {DF_REST_ENABLED, 1, 0}, {DF_COMPLETED, 1, 0}});
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

    fit::FitWriter::Data d = mFit->data(L_RECORD);
    d.u32(unixToFitTimestamp(record.timestamp));
    if (record.heartRate != 0) {
        d.u8(record.heartRate);
    } else {
        d.u8(static_cast<uint8_t>(fit::baseTypeInvalid(fit::BaseType::UInt8)));
    }
    // Developer field, in definition order.
    if (tempPresent(record.watchTempX100)) {
        // Hundredths to a decimal: 3028 is written as 30.28. Integers all the way
        // to here; the only float is the value as the file stores it.
        d.f32(static_cast<float>(record.watchTempX100) / 100.0f);
    } else {
        d.invalid(fit::BaseType::Float32);
    }
    const bool ok = d.write();

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
    d.u8(kLapTriggerManual)
        // Developer fields, in definition order.
        .u8(lap.phase)
        .u8(lap.round);
    if (lap.hasRecovery) {
        d.i16(lap.recoveryBpm);
    } else {
        d.invalid(fit::BaseType::SInt16);
    }
    d.write();
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
        .u8(track.hrMax != 0 ? track.hrMax : static_cast<uint8_t>(fit::baseTypeInvalid(fit::BaseType::UInt8)));
    // The optional standard field sits after the last one declared above and
    // before the developer fields, as the definition orders them.
    if (!mProfileName.empty()) {
        session.str(mProfileName.c_str(), static_cast<uint8_t>(mProfileName.size() + 1u));
    }
    ok = session.u8(track.mode)
        .u8(track.restEnabled)
        .u8(track.completed)
        .write() && ok;

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
    writer.add("activity_type", "sauna");
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

uint32_t ActivityWriter::unixToFitTimestamp(std::time_t unixTimestamp)
{
    const std::time_t FIT_EPOCH_OFFSET = 631065600;
    return static_cast<uint32_t>(unixTimestamp - FIT_EPOCH_OFFSET);
}
