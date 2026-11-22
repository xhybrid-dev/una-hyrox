/**
 ******************************************************************************
 * @file    FamilyReader.hpp
 * @brief   The X family's other figures for Streak's Your numbers screen
 *          (NOTES "S7"), read from the shared folder with each app's own code.
 *
 *   X Training   ../SharedData/HybridX/training_today.json: today's planned
 *                session, picked as X Training's glance picks it
 *                (TodayFile::pick). The primary, else its .bak.
 *   X Run        ../SharedData/HybridX/vo2max.json: the VO2max X Run shows, by
 *                X Training's range-checked reader (RunVo2Max). The primary,
 *                else its .bak.
 *   X Weigh-in   ../SharedData/HybridX/weighins.csv through X Weigh-in's own
 *                Trend, so the trend and the week's change are the ones X
 *                Weigh-in's Home shows; its unit from weighin_units.txt.
 *
 * Those files' code is copied into Libs/Family byte for byte (check-family-
 * copies.sh). Each file is optional: one that is missing or does not read
 * leaves its part of the view empty, and the screen leaves its rows out.
 *
 * Read only, never written: each file has one writer, its own app (the
 * family's rule). Holds a 1.5 KB buffer, one decoded plan window and a 90-day
 * trend, so place it at start-up like the scanner. No heap.
 ******************************************************************************
 */

#ifndef STREAK_FAMILY_READER_HPP
#define STREAK_FAMILY_READER_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "DashView.hpp"
#include "Date.hpp"
#include "LogStore.hpp"
#include "TodayFile.hpp"
#include "Trend.hpp"

namespace Streak
{

class FamilyReader
{
public:
    /// Fill @p out for local day @p today (days since 1970-01-01, as the model's).
    void read(SDK::Interface::IFileSystem& fs, int32_t today, bool clockOk, FamilyView& out);

    // -- The pure steps, for the tests ---------------------------------------------
    /// X Training's yyyymmdd for a local day number; 0 for one out of its range.
    static XTraining::Ymd ymdOf(int32_t localDay);
    /// Today's plan from the file's data (null = no usable file), as the glance picks it.
    static void planFrom(const XTraining::TodayData* data, int32_t today, bool clockOk, PlanToday& out);
    /// The weight from X Weigh-in's trend summary; false (and @p out cleared) with no weigh-in.
    static bool weightFrom(const XWeighIn::Summary& s, XWeighIn::WeightUnit unit, WeightFigure& out);

private:
    bool readPlan(SDK::Interface::IFileSystem& fs);
    bool readVo2(SDK::Interface::IFileSystem& fs, Vo2Figure& out);

    char                 mText[XTraining::TodayFile::kMaxJsonBytes + 1] {};   // + its terminator
    XTraining::TodayData mPlan {};
    XWeighIn::Trend      mTrend {};
};

} // namespace Streak

#endif // STREAK_FAMILY_READER_HPP
