/**
 ******************************************************************************
 * @file    WorkoutStore.hpp
 * @brief   The workouts on the watch: the files in Workouts/, over IFileSystem
 *          (Phase P3b).
 *
 * A workout is always a file (docs/WORKOUT_FILE.md): copied over USB, sent
 * from the phone, or (later) made on the watch. Modelled on HybridX Trail's
 * Navigator, so the host tests run it against an in-memory file system:
 *
 *   - scan(): lists Workouts/ (creating it if missing), parses every .json and
 *     keeps a summary of each, sorted by name. Unlike Trail's GPX files these
 *     are at most 4 KB, so they are simply parsed again on every scan: no
 *     index file.
 *   - load(): reads one runnable workout and remembers the choice in
 *     workout.sel; restoreSelection() brings it back on the next start.
 *
 * Memory is fixed (about 7 KB): construct it once, in static storage, not on
 * the service's 10 KB stack.
 ******************************************************************************
 */

#ifndef INTERVALS_WORKOUT_STORE_HPP
#define INTERVALS_WORKOUT_STORE_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "WorkoutParser.hpp"
#include "WorkoutSummary.hpp"
#include "WorkoutTypes.hpp"

namespace Intervals
{

struct WorkoutInfo {
    char            file[48]   = {};   ///< file name in Workouts/ (Trail's limit and rules)
    char            name[Workout::kNameChars] = {};   ///< from the file, or the file name without ".json"
    Sport           sport      = Sport::Running;
    ParseError      error      = ParseError::Ok;
    ValidationError validation = ValidationError::Ok;   ///< when error == Invalid
    uint32_t        errorAt    = 0;    ///< byte offset of a parse error, for the log
    WorkoutSummary  summary {};
    bool            runnable   = false;   ///< parsed, valid, and a sport this app records
};

class WorkoutStore
{
public:
    static constexpr uint8_t     kMaxWorkouts = 16;
    static constexpr const char* kDir         = "Workouts";
    static constexpr const char* kSelFile     = "workout.sel";

    explicit WorkoutStore(SDK::Interface::IFileSystem& fs);

    /// List Workouts/ (creating it if missing). Returns how many there are.
    /// A workout that was loaded stays loaded if its file is still there and
    /// still runnable; otherwise there is no selection.
    uint8_t            scan();
    uint8_t            count() const { return mCount; }
    const WorkoutInfo* infos() const { return mInfos; }
    /// More .json files than kMaxWorkouts were found (the rest are not listed).
    bool               truncated() const { return mTruncated; }

    /// Load workout @p index from the last scan() and remember it. False (and
    /// nothing loaded) if it is not runnable or cannot be read now.
    bool load(uint8_t index);
    /// Load the workout chosen last time, if it is still there and runnable.
    bool restoreSelection();
    /// No workout loaded (the choice on file is kept, for restoreSelection()).
    void clear();
    /// The athlete chose no workout: clear() and forget the choice on file.
    void forget();

    bool               loaded() const { return mSelected >= 0; }
    int8_t             selected() const { return mSelected; }   ///< -1 when none
    const Workout&     current() const { return mCurrent; }
    const WorkoutInfo& currentInfo() const { return mCurrentInfo; }

    /// How many files have been parsed (for the tests).
    uint16_t parses() const { return mParses; }

private:
    bool readAndParse(const char* file, Workout& out, WorkoutInfo& info);
    void writeSelection(const char* file);

    SDK::Interface::IFileSystem& mFs;

    WorkoutInfo mInfos[kMaxWorkouts] {};
    uint8_t     mCount     = 0;
    bool        mTruncated = false;
    Workout     mScratch {};
    Workout     mCurrent {};
    WorkoutInfo mCurrentInfo {};
    int8_t      mSelected = -1;
    uint16_t    mParses   = 0;

    SDK::Interface::IFileSystem::ObjectInfo mObject {};
    char mPath[SDK::Interface::IFileSystem::skMaxPathLen] {};
    char mBuf[kMaxWorkoutFileBytes] {};
};

} // namespace Intervals

#endif // INTERVALS_WORKOUT_STORE_HPP
