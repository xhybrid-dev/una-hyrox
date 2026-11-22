/**
 ******************************************************************************
 * @file    FamilyReader.cpp
 * @brief   The X family's other figures (see the header).
 ******************************************************************************
 */

#include "FamilyReader.hpp"

#include <cstdio>
#include <cstring>

#include "RunVo2Max.hpp"
#include "SafeFile.hpp"
#include "WeekMath.hpp"

namespace Streak
{

namespace
{
void copyText(char* dst, size_t cap, const char* src)
{
    std::snprintf(dst, cap, "%s", src);
}
} // namespace

XTraining::Ymd FamilyReader::ymdOf(int32_t localDay)
{
    const WeekMath::Civil c = WeekMath::civilFromDays(localDay);
    if (c.year < XTraining::Date::kMinYear || c.year > XTraining::Date::kMaxYear) {
        return 0;
    }
    return XTraining::Date::make(static_cast<int>(c.year), static_cast<int>(c.month), static_cast<int>(c.day));
}

void FamilyReader::planFrom(const XTraining::TodayData* data, int32_t today, bool clockOk, PlanToday& out)
{
    out = PlanToday {};
    const XTraining::Ymd                 ymd = clockOk ? ymdOf(today) : 0;
    const XTraining::TodayFile::Pick     p   = XTraining::TodayFile::pick(data, ymd);
    out.state = static_cast<uint8_t>(p.state);
    if (data != nullptr) {
        out.startDate = data->start;
    }
    if (p.today != nullptr) {
        out.kind = static_cast<uint8_t>(p.today->kind);
        copyText(out.what, sizeof(out.what), p.today->what);
        copyText(out.pace, sizeof(out.pace), p.today->pace);
    }
    if (p.next != nullptr && ymd != 0) {
        out.hasNext      = 1;
        out.nextKind     = static_cast<uint8_t>(p.next->kind);
        out.nextTomorrow = p.next->date == XTraining::Date::addDays(ymd, 1) ? 1 : 0;
        out.nextWeekday  = static_cast<uint8_t>(XTraining::Date::weekday(p.next->date));
        copyText(out.nextWhat, sizeof(out.nextWhat), p.next->what);
    }
}

bool FamilyReader::weightFrom(const XWeighIn::Summary& s, XWeighIn::WeightUnit unit, WeightFigure& out)
{
    out = WeightFigure {};
    if (s.days == 0 || s.trend <= 0) {
        return false;
    }
    out.trend         = s.trend;
    out.weekChange    = s.hasWeekChange ? s.weekChange : 0;
    out.hasWeekChange = s.hasWeekChange ? 1 : 0;
    out.lastDay       = s.lastDay;
    out.days          = static_cast<uint16_t>(s.days > 0xFFFFu ? 0xFFFFu : s.days);
    out.unit          = static_cast<uint8_t>(unit);
    return true;
}

bool FamilyReader::readPlan(SDK::Interface::IFileSystem& fs)
{
    // The app saves by a rename, keeping the old copy as .bak: a look during a save reads that
    // (as X Training's glance does).
    for (int backup = 0; backup < 2; ++backup) {
        const size_t len = SafeFile::read(fs, XTraining::TodayFile::kPath, backup != 0, mText, sizeof(mText));
        if (len > 0 && XTraining::TodayFile::read(mText, len, mPlan)) {
            return true;
        }
    }
    return false;
}

bool FamilyReader::readVo2(SDK::Interface::IFileSystem& fs, Vo2Figure& out)
{
    out = Vo2Figure {};
    for (int backup = 0; backup < 2; ++backup) {
        const size_t len = SafeFile::read(fs, XTraining::RunVo2Max::kPath, backup != 0, mText,
                                          XTraining::RunVo2Max::kMaxBytes + 1);
        XTraining::RunVo2Max::Figure f;
        if (len > 0 && XTraining::RunVo2Max::read(mText, len, f)) {
            out.x10  = f.x10;
            out.runs = f.runs;
            return true;
        }
    }
    return false;
}

void FamilyReader::read(SDK::Interface::IFileSystem& fs, int32_t today, bool clockOk, FamilyView& out)
{
    out         = FamilyView {};
    out.today   = today;
    out.clockOk = clockOk ? 1 : 0;

    // X Training: with no usable file the row is left out (X Training not installed, or never opened).
    if (readPlan(fs)) {
        planFrom(&mPlan, today, clockOk, out.plan);
        out.flags |= FamilyView::kPlan;
    }

    // X Run.
    if (readVo2(fs, out.vo2)) {
        out.flags |= FamilyView::kVo2;
    }

    // X Weigh-in: the whole log through its own trend (it streams; no heap).
    mTrend.reset();
    const XWeighIn::LogStore::ScanResult scan = XWeighIn::LogStore::scan(fs, XWeighIn::LogStore::kSharedFile, mTrend);
    if (scan.opened && weightFrom(mTrend.summary(), XWeighIn::LogStore::readUnit(fs), out.weight)) {
        out.flags |= FamilyView::kWeight;
    }
}

} // namespace Streak
