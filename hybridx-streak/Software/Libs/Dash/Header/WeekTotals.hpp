/**
 ******************************************************************************
 * @file    WeekTotals.hpp
 * @brief   The week by sport: time, sessions and distance per kind, for any run
 *          of days, from the X family's activity summaries (NOTES "S7").
 *
 * LinkReader hands it every valid entry of every summary it reads (it is an
 * ActivitySink); the service then asks for this week's and last week's sums once
 * the model has settled which days those are. Each summary holds an app's last 16
 * activities, so a fortnight is nearly always all there.
 *
 * EVERYTHING THE APPS RECORDED COUNTS, as for the training load: the streak's
 * rules (minimum minutes, scope, a session left out) are not applied, because the
 * body did the work either way. Manual logs carry no time or distance and are
 * counted by the service from the state, not here.
 *
 * Seconds and metres are summed before they are turned into minutes, so ten
 * sessions of 59 seconds make 9 minutes, not 0. Pure, bounded, no heap.
 ******************************************************************************
 */

#ifndef STREAK_WEEK_TOTALS_HPP
#define STREAK_WEEK_TOTALS_HPP

#include <cstddef>
#include <cstdint>

#include "ActivityLink.hpp"
#include "DashView.hpp"
#include "LinkReader.hpp"
#include "StreakTypes.hpp"

namespace Streak
{

class WeekTotals : public ActivitySink
{
public:
    /// Every summary file's entries: HybridXLink::kMaxSlots files of kMaxEntries each.
    static constexpr uint16_t kMax = HybridXLink::kMaxSlots * HybridXLink::kMaxEntries;

    void clear();

    /// ActivitySink: keep one activity (beyond kMax it is dropped and counted).
    void onActivity(int32_t localDay, Kind kind, uint32_t seconds, uint32_t metres, uint16_t load) override;

    /// The activities on days [firstDay, lastDay], by kind.
    WeekSum sum(int32_t firstDay, int32_t lastDay) const;

    uint16_t count() const { return mCount; }
    uint16_t dropped() const { return mDropped; }

private:
    struct Item {
        int32_t  day     = 0;
        uint32_t seconds = 0;
        uint32_t metres  = 0;
        Kind     kind    = Kind::Other;
    };

    Item     mItems[kMax] {};
    uint16_t mCount   = 0;
    uint16_t mDropped = 0;
};

} // namespace Streak

#endif // STREAK_WEEK_TOTALS_HPP
