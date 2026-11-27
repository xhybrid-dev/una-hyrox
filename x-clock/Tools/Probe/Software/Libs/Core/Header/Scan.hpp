/**
 ******************************************************************************
 * @file    Scan.hpp
 * @brief   One look at the shared folder: list it, ask after every file the
 *          family leaves there, and hand each to the family's own reader.
 *
 * It answers, for a Clockface-type app on the watch:
 *   - can it list ../SharedData and ../SharedData/HybridX, and what is there;
 *   - which of the known files exist, how big, how old;
 *   - do they read, and do the family's readers accept what a real watch holds.
 * The readers are copies of the apps' own (Link, Training, Vo2), so "Parsed"
 * means the real face could use the file as it is.
 ******************************************************************************
 */

#ifndef CLOCK_PROBE_SCAN_HPP
#define CLOCK_PROBE_SCAN_HPP

#include <cstddef>
#include <cstdint>

#include "ActivityLink.hpp"
#include "FileProbe.hpp"
#include "GlanceView.hpp"
#include "KnownFiles.hpp"
#include "StreakNow.hpp"
#include "TodayFile.hpp"

namespace ClockProbe
{

enum class Verdict : uint8_t {
    NotThere   = 0,   ///< exist() said no
    CannotRead = 1,   ///< it exists but would not open or read
    Parsed     = 2,   ///< the family's reader accepted it
    BadFormat  = 3,   ///< read, but the reader refused it (or it was cut short)
    SizeOnly   = 4,   ///< asked after, never opened
};

const char* verdictName(Verdict v);

struct ScanFile {
    FileResult r;
    Verdict    verdict = Verdict::NotThere;
};

/// Everything one scan learns, as plain data (about 4.5 KB: keep it off the service's 10 KB stack).
struct ScanData {
    ScanFile   files[kKnownCount];
    ListResult shared;     ///< ../SharedData
    ListResult hybridx;    ///< ../SharedData/HybridX

    HybridXLink::ActivityFile act[kLinkFiles];
    bool                      actOk[kLinkFiles] = {};
    HybridXLink::StreakNow    streakNow;
    bool                      streakNowOk = false;
    XTraining::TodayData      today;
    bool                      todayOk = false;
    RunVo2::Glance::View      vo2;
    bool                      vo2Ok = false;

    /// Reset in place. Not `*this = ScanData{}`: that builds a 4.5 KB temporary, and the service's
    /// stack is 10 KB (Docs/service-lifecycle.md) with the JSON readers still to come.
    void clear();

    /// Files that exist (of kKnownCount).
    uint8_t found() const;
    /// Files that exist but did not read or parse.
    uint8_t problems() const;
};

/// Where a scan sends what it sees as it goes, so the service can log the text without keeping it.
struct ScanSink {
    void (*listed)(void* ctx, const char* dir, const ListEntry& entry) = nullptr;
    /// After each folder's entries: the folder's result (so a log can put its header before its entries).
    void (*listDone)(void* ctx, const char* dir, const ListResult& result) = nullptr;
    /// @p text is the file's content for a file that read (and whose role allows it), else "".
    void (*file)(void* ctx, uint8_t index, const FileResult& r, Verdict v, const char* text) = nullptr;
    void* ctx = nullptr;
};

/// Run a scan. @p scratch holds one file at a time (kScratchBytes is enough).
void runScan(SDK::Interface::IFileSystem& fs, char* scratch, size_t cap, ScanData& out, const ScanSink& sink);

} // namespace ClockProbe

#endif // CLOCK_PROBE_SCAN_HPP
