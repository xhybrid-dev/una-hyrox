/**
 ******************************************************************************
 * @file    FitFixture.hpp
 * @brief   Writes activity .fit files with the SDK's real FitWriter.
 *
 * The streak's reader is tested against what the watch actually writes, not a
 * hand-made imitation: this drives una-sdk's FitWriter (the encoder every SDK
 * activity app uses) with the same message layout as the SDK's
 * ActivityWriter -- file_id, developer_data_id, event, records carrying
 * developer fields, lap, session, activity -- and any FitProfile sport.
 *
 * Used by the host tests (into TreeFileSystem) and by tools/make_fit (to disk,
 * for the simulator's fixture tree).
 ******************************************************************************
 */

#ifndef STREAK_FIT_FIXTURE_HPP
#define STREAK_FIT_FIXTURE_HPP

#include <cstdint>
#include <string>

#include "SDK/Interfaces/IFileSystem.hpp"

namespace Fixture
{

struct FitSpec {
    uint8_t  sport     = 1;    ///< FitProfile Sport (1 = Running)
    uint8_t  subSport  = 0;
    uint32_t startUnix = 0;    ///< UTC start
    uint32_t timerS    = 1800;
    uint16_t records   = 20;   ///< one a second from the start
};

/// Write a complete activity to @p file (opened for writing). True on success.
bool writeFit(SDK::Interface::IFile& file, const FitSpec& spec);

/// The same, into a string (via an in-memory IFile).
std::string fitBytes(const FitSpec& spec);

/// Unix time of a local calendar time, for a UTC offset in minutes.
uint32_t unixOf(int year, int month, int day, int hour, int minute, int second, int offsetMin = 0);

/// "activity_YYYYMMDDTHHMMSS.fit" for a local time, as ActivityWriter names it.
std::string activityName(int year, int month, int day, int hour, int minute, int second);

} // namespace Fixture

#endif // STREAK_FIT_FIXTURE_HPP
