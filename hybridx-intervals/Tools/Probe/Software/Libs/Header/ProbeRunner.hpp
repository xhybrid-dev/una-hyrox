/**
 ******************************************************************************
 * @file    ProbeRunner.hpp
 * @brief   The Intervals Probe's one check, over IFileSystem, with no kernel.
 *
 * Answers hybridx-intervals PLAN P0: after a PC-side BLE client (Tools/Probe/pc)
 * writes a small JSON file into this app's own "Plans/" folder over the BLE
 * File Transfer Service, can this app find and read it?
 *
 *   1. ensure "Plans/" exists (IFileSystem::mkdir is idempotent: it succeeds
 *      whether it creates the folder or finds it already there);
 *   2. list it, bounded, counting files and tracking the newest by mtime;
 *   3. open the newest file and read a safely-printable preview of it.
 *
 * Pure C++ over SDK::Interface::IFileSystem, so the host test runs it against
 * a small in-memory fake (Tests/Host/support/FlatFileSystem) rather than
 * needing a watch.
 ******************************************************************************
 */

#ifndef INTERVALS_PROBE_RUNNER_HPP
#define INTERVALS_PROBE_RUNNER_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "ProbeResult.hpp"

namespace Probe
{

/// Where the report's narration goes.
class Host
{
public:
    /// One line of the report, without its newline.
    virtual void line(const char* text) = 0;

protected:
    ~Host() = default;
};

class Runner
{
public:
    static constexpr uint16_t kMaxEntries = 500;    ///< bounds the directory read
    static constexpr const char* kPlansDir = "Plans";

    Runner(SDK::Interface::IFileSystem& fs, Host& host);

    /// Run the check, reporting through the host; fills every field of @p r
    /// except the service's own (utc, utcOffsetMin, run).
    void run(Result& r);

private:
    void sanitise(const char* in, size_t inLen, char* out, size_t outCap);

    SDK::Interface::IFileSystem& mFs;
    Host&                        mHost;
    SDK::Interface::IFileSystem::ObjectInfo mInfo {};
    char mLine[256] {};
    char mPath[SDK::Interface::IFileSystem::skMaxPathLen] {};
    char mBuf[96] {};
};

} // namespace Probe

#endif // INTERVALS_PROBE_RUNNER_HPP
