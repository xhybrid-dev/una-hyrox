/**
 ******************************************************************************
 * @file    GlanceView.cpp
 * @brief   What X Training's glance has to say (see the header).
 ******************************************************************************
 */

#include "GlanceView.hpp"

#include <cstring>

namespace TrainingGlance
{

using namespace XTraining;

namespace
{
void copy(char* dst, size_t cap, const char* src)
{
    std::strncpy(dst, src, cap - 1);
    dst[cap - 1] = '\0';
}

Pip pipFor(const TodayDay* row, Ymd date, Ymd today)
{
    if (row == nullptr) {
        return Pip::None;
    }
    if (row->kind == Kind::Rest) {
        return Pip::Rest;
    }
    switch (row->status) {
    case TodayStatus::Done:
    case TodayStatus::PartDone:
        return Pip::Done;
    case TodayStatus::Missed:
    case TodayStatus::Skipped:
        return Pip::Missed;
    case TodayStatus::ToCome:
        break;
    }
    if (date < today) {
        return Pip::Missed;   // passed with nothing logged: the app settles it when opened
    }
    return date == today ? Pip::Today : Pip::ToCome;
}
}  // namespace

void makeView(const TodayData* data, Ymd today, View& out)
{
    out = View{};
    const TodayFile::Pick p = TodayFile::pick(data, today);
    out.state = p.state;
    if (data == nullptr) {
        return;
    }
    out.startDate = data->start;
    if (p.next != nullptr) {
        out.hasNext      = true;
        out.nextTomorrow = p.next->date == Date::addDays(today, 1);
        out.nextWeekday  = static_cast<uint8_t>(Date::weekday(p.next->date));
        out.nextKind     = p.next->kind;
        copy(out.nextWhat, sizeof(out.nextWhat), p.next->what);
    }
    if (p.today == nullptr) {
        return;   // only a session, done, skipped or a rest day has a row of its own
    }
    out.kind = p.today->kind;
    copy(out.what, sizeof(out.what), p.today->what);
    copy(out.pace, sizeof(out.pace), p.today->pace);

    // The week of seven days, Monday to Sunday, and where the plan has got to.
    out.hasWeek = true;
    out.weekNo  = p.today->runWeek;
    out.weeks   = data->weeks;
    const DayNum monday = Date::mondayOf(Date::toDayNum(today));
    for (int i = 0; i < 7; ++i) {
        const Ymd date = Date::fromDayNum(monday + i);
        out.pip[i] = pipFor(data->find(date), date, today);
    }
}

void makeView(const char* text, size_t len, Ymd today, View& out)
{
    TodayData* data = nullptr;
    static TodayData sData;   // 1.1 KB: off the glance service's 10 KB stack, and not on the heap
    if (text != nullptr && len > 0 && len <= kMaxFileBytes && TodayFile::read(text, len, sData)) {
        data = &sData;
    }
    makeView(data, today, out);
}

}  // namespace TrainingGlance
