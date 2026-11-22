/**
 ******************************************************************************
 * @file    WeekTotals.cpp
 * @brief   The week by sport (see the header).
 ******************************************************************************
 */

#include "WeekTotals.hpp"

namespace Streak
{

namespace
{
/// The largest a sum's parts may reach: they are kept in 16 and 32 bits.
constexpr uint64_t kMaxMinutes = 0xFFFFu;
constexpr uint64_t kMaxMetres  = 0xFFFFFFFFu;
constexpr uint8_t  kMaxCount   = 0xFFu;

struct Acc {
    uint64_t seconds  = 0;
    uint64_t metres   = 0;
    uint16_t sessions = 0;

    void add(uint32_t s, uint32_t m)
    {
        seconds += s;
        metres += m;
        ++sessions;
    }

    SportSum out() const
    {
        SportSum r;
        const uint64_t minutes = seconds / 60u;
        r.minutes  = static_cast<uint16_t>(minutes > kMaxMinutes ? kMaxMinutes : minutes);
        r.metres   = static_cast<uint32_t>(metres > kMaxMetres ? kMaxMetres : metres);
        r.sessions = static_cast<uint8_t>(sessions > kMaxCount ? kMaxCount : sessions);
        return r;
    }
};
} // namespace

void WeekTotals::clear()
{
    mCount   = 0;
    mDropped = 0;
}

void WeekTotals::onActivity(int32_t localDay, Kind kind, uint32_t seconds, uint32_t metres, uint16_t /*load*/)
{
    if (mCount >= kMax) {
        ++mDropped;
        return;
    }
    Item& it   = mItems[mCount++];
    it.day     = localDay;
    it.seconds = seconds;
    it.metres  = metres;
    it.kind    = static_cast<uint8_t>(kind) < kKindCount ? kind : Kind::Other;
}

WeekSum WeekTotals::sum(int32_t firstDay, int32_t lastDay) const
{
    Acc kinds[kKindCount];
    Acc all;
    for (uint16_t i = 0; i < mCount; ++i) {
        const Item& it = mItems[i];
        if (it.day < firstDay || it.day > lastDay) {
            continue;
        }
        kinds[static_cast<uint8_t>(it.kind)].add(it.seconds, it.metres);
        all.add(it.seconds, it.metres);
    }
    WeekSum out;
    for (uint8_t k = 0; k < kKindCount; ++k) {
        out.kind[k] = kinds[k].out();
    }
    out.all = all.out();
    return out;
}

} // namespace Streak
