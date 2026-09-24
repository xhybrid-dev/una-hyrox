/**
 ******************************************************************************
 * @file    FitFixture.cpp
 * @brief   Activity .fit files from the SDK's FitWriter (see the header).
 ******************************************************************************
 */

#include "FitFixture.hpp"

#include <cstdio>
#include <algorithm>
#include <cstring>

#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Fit/FitWriter.hpp"
#include "WeekMath.hpp"

namespace fit = SDK::Fit;

namespace Fixture
{

namespace
{
constexpr uint32_t kFitEpochOffset = 631065600u;   // ActivityWriter::skFitEpochOffset

uint32_t toFit(uint32_t unixSeconds)
{
    return unixSeconds - kFitEpochOffset;
}

/// A growable in-memory IFile, for fitBytes().
class StringFile : public SDK::Interface::IFile
{
public:
    std::string data;

    void setPath(const char*) override {}
    const char* getPath() const override { return "mem.fit"; }
    bool exist() const override { return true; }
    bool rename(const char*) override { return false; }
    bool remove() override { return false; }
    size_t size() const override { return data.size(); }
    bool open(bool, bool override) override
    {
        if (override) {
            data.clear();
        }
        mPos = 0;
        return true;
    }
    bool isOpen() const override { return true; }
    bool close() override { return true; }
    bool read(char* buff, size_t btr, size_t& br) override
    {
        br = mPos < data.size() ? std::min(btr, data.size() - mPos) : 0;
        std::memcpy(buff, data.data() + mPos, br);
        mPos += br;
        return true;
    }
    bool write(const char* buff, size_t btw, size_t& bw) override
    {
        if (mPos > data.size()) {
            data.resize(mPos);
        }
        data.replace(mPos, btw, buff, btw);
        mPos += btw;
        bw = btw;
        return true;
    }
    bool seek(size_t offset) override { mPos = offset; return true; }
    bool truncate(size_t offset) override { data.resize(offset); return true; }
    bool flush() override { return true; }
    size_t getPosition() const override { return mPos; }

private:
    size_t mPos = 0;
};

enum : uint8_t { L_FILE_ID, L_DEV_ID, L_FIELD_DESC, L_EVENT, L_RECORD, L_LAP, L_SESSION, L_ACTIVITY };
} // namespace

bool writeFit(SDK::Interface::IFile& file, const FitSpec& spec)
{
    fit::FitWriter w(file);
    bool ok = w.begin(0);

    const uint8_t nameLen = static_cast<uint8_t>(std::strlen(fit::kProductName) + 1);
    ok = ok && w.defineMessage(L_FILE_ID, fit::mesgNum(fit::MesgNum::FileId),
                               {fit::field::FileId::Type, fit::field::FileId::Manufacturer,
                                fit::field::FileId::Product, fit::field::FileId::SerialNumber,
                                fit::field::FileId::TimeCreated,
                                {fit::field::FileId::kProductNameNum, fit::BaseType::String, nameLen}});
    ok = ok && w.data(L_FILE_ID)
                   .u8(static_cast<uint8_t>(fit::File::Activity))
                   .u16(static_cast<uint16_t>(fit::Manufacturer::Una))
                   .u16(static_cast<uint16_t>(fit::Product::UnaWatch))
                   .u32(0)
                   .u32(toFit(spec.startUnix))
                   .str(fit::kProductName, nameLen)
                   .write();

    ok = ok && w.defineMessage(L_DEV_ID, fit::mesgNum(fit::MesgNum::DeveloperDataId),
                               {fit::field::DeveloperDataId::ApplicationId,
                                fit::field::DeveloperDataId::DeveloperDataIndex});
    uint8_t appId[16] = {'F', 'i', 'x', 't', 'u', 'r', 'e'};
    ok = ok && w.data(L_DEV_ID).bytes(appId, sizeof(appId)).u8(0).write();

    // Descriptions of the three developer fields the records carry.
    constexpr uint8_t kNameLen = 12;
    ok = ok && w.defineMessage(L_FIELD_DESC, fit::mesgNum(fit::MesgNum::FieldDescription),
                               {fit::field::FieldDescription::DeveloperDataIndex,
                                fit::field::FieldDescription::FieldDefinitionNumber,
                                fit::field::FieldDescription::FitBaseTypeId,
                                {fit::field::FieldDescription::kFieldNameNum, fit::BaseType::String, kNameLen}});
    const char* kNames[3] = {"hr_source", "hr_optical", "hr_external"};
    for (uint8_t i = 0; i < 3; ++i) {
        ok = ok && w.data(L_FIELD_DESC).u8(0).u8(i).u8(static_cast<uint8_t>(fit::BaseType::UInt8))
                       .str(kNames[i], kNameLen).write();
    }

    ok = ok && w.defineMessage(L_EVENT, fit::mesgNum(fit::MesgNum::Event),
                               {fit::field::Event::Timestamp, fit::field::Event::EventField,
                                fit::field::Event::EventType});
    ok = ok && w.data(L_EVENT)
                   .u32(toFit(spec.startUnix))
                   .u8(static_cast<uint8_t>(fit::Event::Timer))
                   .u8(static_cast<uint8_t>(fit::EventType::Start))
                   .write();

    // Records with three 1-byte developer fields, as ActivityWriter's hr3 set.
    ok = ok && w.defineMessage(L_RECORD, fit::mesgNum(fit::MesgNum::Record),
                               {fit::field::Record::Timestamp, fit::field::Record::HeartRate,
                                fit::field::Record::Distance},
                               {{0, 1, 0}, {1, 1, 0}, {2, 1, 0}});
    for (uint16_t i = 0; i < spec.records; ++i) {
        ok = ok && w.data(L_RECORD)
                       .u32(toFit(spec.startUnix + i))
                       .u8(static_cast<uint8_t>(120 + i % 40))
                       .u32(i * 300u)
                       .u8(1).u8(static_cast<uint8_t>(120 + i % 40)).u8(0xFF)
                       .write();
    }

    const uint32_t end = spec.startUnix + spec.timerS;
    ok = ok && w.defineMessage(L_LAP, fit::mesgNum(fit::MesgNum::Lap),
                               {fit::field::Lap::Timestamp, fit::field::Lap::StartTime,
                                fit::field::Lap::TotalElapsedTime, fit::field::Lap::TotalTimerTime,
                                fit::field::Lap::MessageIndex});
    ok = ok && w.data(L_LAP)
                   .u32(toFit(end)).u32(toFit(spec.startUnix))
                   .u32(spec.timerS * 1000u).u32(spec.timerS * 1000u).u16(0)
                   .write();

    ok = ok && w.defineMessage(L_SESSION, fit::mesgNum(fit::MesgNum::Session),
                               {fit::field::Session::Timestamp, fit::field::Session::StartTime,
                                fit::field::Session::TotalElapsedTime, fit::field::Session::TotalTimerTime,
                                fit::field::Session::TotalDistance, fit::field::Session::MessageIndex,
                                fit::field::Session::NumLaps, fit::field::Session::Sport,
                                fit::field::Session::SubSport, fit::field::Session::AvgHeartRate});
    ok = ok && w.data(L_SESSION)
                   .u32(toFit(end)).u32(toFit(spec.startUnix))
                   .u32((spec.timerS + 60u) * 1000u).u32(spec.timerS * 1000u)
                   .u32(spec.timerS * 300u).u16(0).u16(1)
                   .u8(spec.sport).u8(spec.subSport).u8(140)
                   .write();

    ok = ok && w.defineMessage(L_ACTIVITY, fit::mesgNum(fit::MesgNum::Activity),
                               {fit::field::Activity::Timestamp, fit::field::Activity::TotalTimerTime,
                                fit::field::Activity::LocalTimestamp, fit::field::Activity::NumSessions});
    ok = ok && w.data(L_ACTIVITY).u32(toFit(end)).u32(spec.timerS * 1000u).u32(toFit(end)).u16(1).write();

    return ok && w.finish();
}

std::string fitBytes(const FitSpec& spec)
{
    StringFile f;
    f.open(true, true);
    writeFit(f, spec);
    return f.data;
}

uint32_t unixOf(int year, int month, int day, int hour, int minute, int second, int offsetMin)
{
    const int64_t days = Streak::WeekMath::daysFromCivil(year, static_cast<uint32_t>(month), static_cast<uint32_t>(day));
    const int64_t local = days * 86400 + hour * 3600 + minute * 60 + second;
    return static_cast<uint32_t>(local - offsetMin * 60);
}

std::string activityName(int year, int month, int day, int hour, int minute, int second)
{
    char name[48];
    snprintf(name, sizeof(name), "activity_%04d%02d%02dT%02d%02d%02d.fit", year, month, day, hour, minute, second);
    return name;
}

} // namespace Fixture
